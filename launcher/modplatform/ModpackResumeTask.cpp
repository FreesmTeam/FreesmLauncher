// SPDX-License-Identifier: GPL-3.0-only

#include "ModpackResumeTask.h"

#include <QFile>
#include <QFileInfo>

#include "minecraft/MinecraftInstance.h"
#include "modplatform/flame/FlamePackInstallTask.h"
#include "modplatform/helpers/PendingModpackManifest.h"
#include "modplatform/modrinth/ModrinthPackInstallTask.h"

namespace {
bool markerExists(const QString& path)
{
    const QFileInfo info(path);
    return info.exists() || info.isSymLink();
}
}  // namespace

ModpackResumeTask::ModpackResumeTask(MinecraftInstance* instance, bool explicitlyOffline, QWidget* parent)
    : m_instance(instance), m_explicitlyOffline(explicitlyOffline), m_parent(parent)
{
    setAbortable(true);
}

bool ModpackResumeTask::hasPendingManifest(const QString& instanceRoot)
{
    return markerExists(Modpack::pendingManifestPath(instanceRoot, Modpack::Format::Modrinth)) ||
           markerExists(Modpack::pendingManifestPath(instanceRoot, Modpack::Format::CurseForge));
}

void ModpackResumeTask::executeTask()
{
    m_aborted = false;
    m_task.reset();
    const auto modrinthPath = Modpack::pendingManifestPath(m_instance->instanceRoot(), Modpack::Format::Modrinth);
    const auto curseForgePath = Modpack::pendingManifestPath(m_instance->instanceRoot(), Modpack::Format::CurseForge);
    const bool hasModrinth = markerExists(modrinthPath);
    const bool hasCurseForge = markerExists(curseForgePath);
    if (!hasModrinth && !hasCurseForge) {
        emitSucceeded();
        return;
    }
    if (m_explicitlyOffline) {
        emitFailed(tr("This modpack installation is incomplete. Launch online to finish installing it before playing."));
        return;
    }
    if (hasModrinth && hasCurseForge) {
        emitFailed(
            tr("Cannot resume this modpack because both Modrinth and CurseForge remaining manifests exist. "
               "Resolve the conflicting manifests before launching."));
        return;
    }

    const auto format = hasModrinth ? Modpack::Format::Modrinth : Modpack::Format::CurseForge;
    m_pendingPath = hasModrinth ? modrinthPath : curseForgePath;
    QString error;
    const auto manifest = Modpack::PendingManifest::load(format, m_pendingPath, &error);
    if (!manifest) {
        emitFailed(tr("Could not read the remaining modpack manifest at %1:\n%2").arg(m_pendingPath, error));
        return;
    }

    if (hasModrinth) {
        m_task =
            makeShared<ModrinthPackInstallTask>(m_instance, manifest->remaining(), Modpack::InstallMode::Resume, false, m_parent.data());
    } else {
        m_task = makeShared<FlamePackInstallTask>(m_instance, manifest->remaining(), Modpack::InstallMode::Resume, false, m_parent.data());
    }
    propagateFromOther(m_task.get());
    connect(m_task.get(), &Task::abortStatusChanged, this, &Task::setAbortable);
    connect(m_task.get(), &Task::abortButtonTextChanged, this, &Task::setAbortButtonText);
    connect(m_task.get(), &Task::warningLogged, this, &ModpackResumeTask::logWarning);
    connect(m_task.get(), &Task::finished, this, &ModpackResumeTask::installFinished);
    setAbortable(m_task->canAbort());
    setStatus(tr("Resuming modpack installation"));
    m_task->start();
}

void ModpackResumeTask::installFinished()
{
    if (!isRunning())
        return;

    if (m_aborted || m_task->getState() == Task::State::AbortedByUser) {
        emitAborted();
        return;
    }
    if (!m_task->wasSuccessful()) {
        emitFailed(tr("Could not finish installing the modpack:\n%1").arg(m_task->failReason()));
        return;
    }

    QFile marker(m_pendingPath);
    if (!marker.remove()) {
        emitFailed(tr("The modpack files were installed, but the remaining manifest at %1 could not be removed:\n%2")
                       .arg(m_pendingPath, marker.errorString()));
        return;
    }
    emitSucceeded();
}

bool ModpackResumeTask::canAbort() const
{
    return m_task ? m_task->canAbort() : Task::canAbort();
}

bool ModpackResumeTask::abort()
{
    if (!isRunning() || !canAbort())
        return false;

    if (!m_task)
        return Task::abort();

    m_aborted = true;
    if (!m_task->abort()) {
        m_aborted = false;
        return false;
    }
    return true;
}
