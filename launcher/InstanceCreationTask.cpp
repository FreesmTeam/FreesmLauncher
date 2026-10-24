#include "InstanceCreationTask.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

#include "FileSystem.h"
#include "InstanceTask.h"
#include "minecraft/MinecraftLoadAndCheck.h"
#include "minecraft/PackProfile.h"
#include "minecraft/update/AssetUpdateTask.h"
#include "minecraft/update/LegacyFMLLibrariesTask.h"
#include "minecraft/update/LibrariesTask.h"
#include "settings/Setting.h"
#include "tasks/SequentialTask.h"

bool InstanceCreationTask::abort()
{
    if (!canAbort() || !isRunning()) {
        return false;
    }

    m_abort = true;
    if (m_gameFilesTask) {
        return m_gameFilesTask->abort();
    }

    // createInstance() may be running a nested event loop. Let it stop its content
    // task and decide whether to keep the instance before staging can be removed.
    return true;
}

void InstanceCreationTask::executeTask()
{
    m_abort = false;
    m_contentIncomplete = false;
    m_gameDownloadFailed = false;
    m_error_message.clear();
    m_gameFilesTask.reset();
    m_instance.reset();
    setAbortable(true);

    auto updated = updateInstance();
    if (!isRunning())
        return;
    if (updated) {
        if (m_abort)
            emitAborted();
        else
            emitSucceeded();
        return;
    }

    // When the user aborted in the update stage.
    if (m_abort) {
        emitAborted();
        return;
    }

    m_instance = createInstance();
    if (!isRunning())
        return;
    if (!m_instance) {
        if (m_abort && m_error_message.isEmpty()) {
            emitAborted();
            return;
        }

        qWarning() << "Instance creation failed!";
        if (!m_error_message.isEmpty()) {
            qWarning() << "Reason:" << m_error_message;
            emitFailed(tr("Error while creating new instance:\n%1").arg(m_error_message));
        } else {
            emitFailed(tr("Error while creating new instance."));
        }

        return;
    }

    if (m_contentIncomplete && !shouldOverride()) {
        finishIncompleteInstance(
            tr("Modpack installation is incomplete. Remaining files must be downloaded before playing.")
        );
        return;
    }
    if (m_abort) {
        // Destroy the profile before InstanceStaging removes its directory.
        m_instance.reset();
        emitAborted();
        return;
    }

    // If this is set, it means we're updating an instance. So, we now need to remove the
    // files scheduled to, and we'd better not let the user abort in the middle of it, since it'd
    // put the instance in an invalid state.
    if (shouldOverride()) {
        bool deleteFailed = false;

        setAbortable(false);
        setStatus(tr("Removing old conflicting files..."));
        qDebug() << "Removing old files";

        for (const QString& path : m_filesToRemove) {
            if (!QFile::exists(path))
                continue;

            qDebug() << "Removing" << path;

            if (!QFile::remove(path)) {
                qCritical() << "Could not remove" << path;
                deleteFailed = true;
            }
        }

        if (deleteFailed) {
            emitFailed(tr("Failed to remove old conflicting files."));
            return;
        }
    }

    if (!m_abort) {
        setAbortable(true);
        setAbortButtonText(tr("Skip"));
        qDebug() << "Downloading game files";

        auto updateTasks = m_instance->createUpdateTask();
        if (updateTasks.isEmpty()) {
            emitSucceeded();
            return;
        }
        auto task = makeShared<SequentialTask>();
        auto loadTask = makeShared<MinecraftLoadAndCheck>(m_instance.get(), Net::Mode::Online);
        connect(loadTask.get(), &Task::failed, this, [this, source = loadTask.get()] { m_gameDownloadFailed = source->downloadFailed(); });
        task->addTask(loadTask);
        for (const auto& t : updateTasks) {
            // These connections must run before SequentialTask forwards the failure.
            if (auto source = qobject_cast<LibrariesTask*>(t.get())) {
                connect(source, &Task::failed, this, [this, source] { m_gameDownloadFailed = source->downloadFailed(); });
            } else if (auto source = qobject_cast<LegacyFMLLibrariesTask*>(t.get())) {
                connect(source, &Task::failed, this, [this, source] { m_gameDownloadFailed = source->downloadFailed(); });
            } else if (auto source = qobject_cast<AssetUpdateTask*>(t.get())) {
                connect(source, &Task::failed, this, [this, source] { m_gameDownloadFailed = source->downloadFailed(); });
            }
            task->addTask(t);
        }
        connect(task.get(), &Task::finished, this, [this, task] {
            if (!isRunning())
                return;
            if (task->wasSuccessful() || m_abort) {
                emitSucceeded();
            } else if (!shouldOverride() && keepOnGameDownloadFailure() && m_gameDownloadFailed) {
                finishIncompleteInstance(tr("Game file downloads did not finish. The modpack content is complete, but game files must be "
                                            "downloaded before playing.\n%1")
                                             .arg(task->failReason()));
            } else {
                emitFailed(tr("Could not download game files: %1").arg(task->failReason()));
            }
        });
        propagateFromOther(task.get());
        setDetails(tr("Downloading game files"));

        m_gameFilesTask = task;
        m_gameFilesTask->start();
    }
}

void InstanceCreationTask::finishIncompleteInstance(const QString& warning)
{
    setAbortable(false);
    auto settings = m_instance->settings();
    m_instance->saveNow();

    // Instance settings are saved synchronously by INISettingsObject. Check the
    // persisted identity as well: its setters do not return disk write errors.
    QSettings config(FS::PathCombine(m_instance->instanceRoot(), "instance.cfg"), QSettings::IniFormat);
    config.setFallbacksEnabled(false);
    config.sync();
    bool configSaved = QFile::exists(config.fileName()) && config.status() == QSettings::NoError;
    for (const auto& key : { "name", "InstanceType", "iconKey", "ManagedPack", "ManagedPackType", "ManagedPackID", "ManagedPackName",
                             "ManagedPackVersionID", "ManagedPackVersionName" }) {
        auto setting = settings->getSetting(key);
        if (config.value(key, setting->defValue()).toString() != setting->get().toString())
            configSaved = false;
    }
    // saveNow() has no result API. Check the saved component configuration using
    // public profile accessors, without reloading or starting metadata downloads.
    auto profile = m_instance->getPackProfile();
    QFile profileFile(FS::PathCombine(m_instance->instanceRoot(), "mmc-pack.json"));
    QJsonObject savedProfile;
    if (profileFile.open(QIODevice::ReadOnly)) {
        savedProfile = QJsonDocument::fromJson(profileFile.readAll()).object();
        profileFile.close();
    }
    auto savedComponents = savedProfile.value("components").toArray();
    bool profileSaved = savedProfile.value("formatVersion").toInt() == 1 && savedProfile.value("components").isArray() &&
                        savedComponents.size() == profile->rowCount();
    for (int i = 0; profileSaved && i < profile->rowCount(); ++i) {
        auto component = profile->getComponent(static_cast<size_t>(i));
        auto saved = savedComponents[i].toObject();
        profileSaved = saved.value("uid").toString() == component->m_uid && saved.value("version").toString() == component->m_version &&
                       saved.value("dependencyOnly").toBool() == component->m_dependencyOnly &&
                       saved.value("important").toBool() == component->m_important &&
                       saved.value("disabled").toBool() == component->m_disabled;
    }
    if (!configSaved || !profileSaved) {
        m_instance.reset();
        emitFailed(tr("Could not save the incomplete instance configuration."));
        return;
    }

    logWarning(warning);
    emitSucceeded();
}

void InstanceCreationTask::scheduleToDelete(QWidget* parent, QDir dir, QString path, bool checkDisabled)
{
    if (path.isEmpty()) {
        return;
    }
    if (path.startsWith("saves/")) {
        if (m_shouldDeleteSaves == ShouldDeleteSaves::NotAsked) {
            m_shouldDeleteSaves = askIfShouldDeleteSaves(parent);
        }
        if (m_shouldDeleteSaves == ShouldDeleteSaves::No) {
            return;
        }
    }
    qDebug() << "Scheduling" << path << "for removal";
    m_filesToRemove.append(dir.absoluteFilePath(path));
    if (checkDisabled) {
        if (path.endsWith(".disabled")) {  // remove it if it was enabled/disabled by user
            m_filesToRemove.append(dir.absoluteFilePath(path.chopped(9)));
        } else {
            m_filesToRemove.append(dir.absoluteFilePath(path + ".disabled"));
        }
    }
}
