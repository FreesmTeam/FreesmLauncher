// SPDX-License-Identifier: GPL-3.0-only
#include "ModrinthPackInstallTask.h"

#include <QCryptographicHash>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QSet>
#include <QTimer>

#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"
#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/Mod.h"
#include "modplatform/EnsureMetadataTask.h"
#include "net/ApiDownload.h"
#include "net/ChecksumValidator.h"
#include "ui/dialogs/UntrustedModsDialog.h"
#include "ui/pages/modplatform/OptionalModDialog.h"

namespace {
// NetJob::abort() emits its terminal signal before its requests have necessarily
// closed their sinks. Drain them instead, including starts already queued by ConcurrentTask.
class DrainingNetJob final : public NetJob {
   public:
    using NetJob::NetJob;

    bool abort() override
    {
        m_stopping = true;
        m_queue.clear();
        setAskRetry(false);
        QTimer::singleShot(0, this, [this] {
            const auto active = m_doing.values();
            for (const auto& task : active) {
                task->abort();
            }
            executeNextSubTask();
        });
        return true;
    }

   protected:
    void executeNextSubTask() override
    {
        if (!isRunning()) {
            return;
        }
        if (m_stopping) {
            if (m_doing.isEmpty()) {
                emitAborted();
            }
            return;
        }
        NetJob::executeNextSubTask();
    }

   private:
    bool m_stopping = false;
};

bool matchesHash(const QString& path, const QByteArray& hash)
{
    QFile file(path);
    QCryptographicHash hasher(QCryptographicHash::Sha512);
    return file.open(QIODevice::ReadOnly) && hasher.addData(&file) && hasher.result() == hash;
}
}  // namespace

ModrinthPackInstallTask::ModrinthPackInstallTask(MinecraftInstance* instance,
                                                 QJsonObject manifest,
                                                 Modpack::InstallMode mode,
                                                 bool trustedSource,
                                                 QWidget* parent)
    : m_instance(instance)
    , m_pending(Modpack::Format::Modrinth, std::move(manifest))
    , m_mode(mode)
    , m_trustedSource(trustedSource)
    , m_parent(parent)
{
    setAbortable(true);
}

ModrinthPackInstallTask::~ModrinthPackInstallTask() = default;

QJsonObject ModrinthPackInstallTask::remainingManifest() const
{
    return m_pending.remaining();
}

bool ModrinthPackInstallTask::canPreserve() const
{
    return !m_isUpdate && m_validated && isFinished();
}

bool ModrinthPackInstallTask::abort()
{
    if (!isRunning()) {
        return false;
    }
    m_aborted = true;
    if (m_downloadJob && m_downloadJob->isRunning()) {
        m_downloadJob->abort();
    }
    // EnsureMetadataTask::abort disconnects its completion signals. Let this
    // display-only work settle instead of committing a directory it still uses.
    return true;
}

bool ModrinthPackInstallTask::validatePath(const QString& path) const
{
    const auto root = QDir::cleanPath(QFileInfo(m_instance->gameRoot()).absoluteFilePath());
    const auto target = QDir::cleanPath(FS::PathCombine(root, path));
    if (path.isEmpty() || QDir::isAbsolutePath(path) || path.contains(':') || path.contains(QChar::Null) ||
        path.split('/').contains("..") || !target.startsWith(root + '/')) {
        return false;
    }
    // Check every existing component, not just lexical containment: overrides or
    // a user-edited resume directory may contain a symlink to outside the instance.
    auto current = target;
    while (current.startsWith(root + '/') || current == root) {
        if (QFileInfo(current).isSymLink()) {
            return false;
        }
        if (current == root) {
            break;
        }
        current = QFileInfo(current).absolutePath();
    }
    return true;
}

bool ModrinthPackInstallTask::prepare()
{
    const auto manifest = m_pending.remaining();
    QStringList optionalPaths;
    QStringList untrusted;
    QSet<QString> targets;
    try {
        const auto pendingPath = Modpack::pendingManifestPath(m_instance->instanceRoot(), Modpack::Format::Modrinth);
        if (!validatePath("mods/.index") || QFileInfo(pendingPath).isSymLink() ||
            QFileInfo(QFileInfo(pendingPath).absolutePath()).isSymLink()) {
            throw JSONValidationError(tr("The modpack directory contains an unsafe symbolic link."));
        }
        if (Json::requireInteger(manifest, "formatVersion") != 1 || Json::requireString(manifest, "game") != "minecraft") {
            throw JSONValidationError(tr("Unsupported Modrinth pack format."));
        }
        const auto dependencies = Json::requireObject(manifest, "dependencies");
        Json::requireString(dependencies, "minecraft");
        for (auto it = dependencies.begin(); it != dependencies.end(); ++it) {
            if (it.key() != "minecraft" && it.key() != "fabric-loader" && it.key() != "quilt-loader" && it.key() != "forge" &&
                it.key() != "neoforge") {
                throw JSONValidationError(tr("Unknown dependency type: %1").arg(it.key()));
            }
            Json::requireString(it.value(), it.key());
        }
        const auto entries = Json::requireIsArrayOf<QJsonObject>(manifest, "files");
        for (const auto& entry : entries) {
            File file;
            file.key = Json::requireString(entry, "path");
            const auto path = FS::RemoveInvalidPathChars(QString(file.key).replace('\\', '/'));
            auto env = entry["env"].toObject();
            const auto support = env.isEmpty() ? QStringLiteral("required") : env["client"].toString("unsupported");
            file.optional = support == "optional";
            file.unsupported = support == "unsupported";
            if (!validatePath(path)) {
                throw JSONValidationError(tr("Unsafe file path: %1").arg(file.key));
            }
            file.enabledPath = QDir::cleanPath(path);
            file.disabledPath = file.enabledPath + ".disabled";
            if (file.optional && !validatePath(file.disabledPath)) {
                throw JSONValidationError(tr("Unsafe file path: %1").arg(file.key));
            }
            if (targets.contains(file.enabledPath)) {
                throw JSONValidationError(tr("Unsafe or duplicate file path: %1").arg(file.key));
            }
            targets.insert(file.enabledPath);
            if (file.optional) {
                if (targets.contains(file.disabledPath)) {
                    throw JSONValidationError(tr("Duplicate file path: %1").arg(file.key));
                }
                targets.insert(file.disabledPath);
            }
            // Unsupported client entries never become impossible remaining downloads.
            if (!file.unsupported) {
                auto hashes = Json::requireObject(entry, "hashes");
                const auto sha512 = Json::requireString(hashes, "sha512").toLatin1();
                file.hash = QByteArray::fromHex(sha512);
                if (sha512.size() != 128 || file.hash.size() != 64 || file.hash.toHex() != sha512.toLower()) {
                    throw JSONValidationError(tr("Invalid SHA-512 hash for %1").arg(file.key));
                }
                for (const auto& value : Json::requireArray(entry, "downloads")) {
                    const QUrl url(Json::requireString(value, "download URL"));
                    if (url.isValid()) {
                        file.downloads.enqueue(url);
                        if (url.scheme() != "https" || url.host() != BuildConfig.MODRINTH_DOWNLOAD_HOST) {
                            untrusted.append(file.enabledPath);
                        }
                    }
                }
                if (file.downloads.isEmpty()) {
                    throw JSONValidationError(tr("The file '%1' is missing a download link.").arg(file.key));
                }
                if (file.optional) {
                    optionalPaths.append(file.enabledPath);
                }
            }
            m_files.push_back(std::move(file));
        }
    } catch (const JSONValidationError& e) {
        m_error = tr("Could not understand pack index:\n%1").arg(e.cause());
        return false;
    }

    if (!m_trustedSource) {
        const QDir root(m_instance->gameRoot());
        QDirIterator iter(m_instance->modsRoot(), QDir::Files, QDirIterator::Subdirectories);
        while (iter.hasNext()) {
            untrusted.append(root.relativeFilePath(iter.next()));
        }
        untrusted.removeDuplicates();
        if (!untrusted.isEmpty()) {
            UntrustedModsDialog dialog(untrusted, m_parent);
            if (dialog.exec() != QDialog::Accepted) {
                m_aborted = true;
                return false;
            }
        }
    }
    if (m_aborted) {
        return false;
    }
    if (!optionalPaths.isEmpty()) {
        OptionalModDialog dialog(m_parent, optionalPaths);
        if (dialog.exec() != QDialog::Accepted) {
            m_aborted = true;
            return false;
        }
        const auto selected = dialog.getResult();
        for (auto& file : m_files) {
            if (file.optional) {
                file.enabled = selected.contains(file.enabledPath);
            }
        }
    }
    if (m_aborted) {
        return false;
    }
    m_validated = true;
    return true;
}

bool ModrinthPackInstallTask::saveProgress()
{
    if (m_mode != Modpack::InstallMode::Resume) {
        return true;
    }
    QString error;
    if (!m_pending.save(Modpack::pendingManifestPath(m_instance->instanceRoot(), Modpack::Format::Modrinth), &error)) {
        m_error = tr("Could not save remaining modpack files: %1").arg(error);
        return false;
    }
    return true;
}

bool ModrinthPackInstallTask::completeFile(File& file)
{
    // Optional selection changes must not leave enabled and disabled duplicates.
    if (file.optional) {
        const auto other = FS::PathCombine(m_instance->gameRoot(), file.alternatePath());
        if (QFile::exists(other) && !QFile::remove(other)) {
            m_error = tr("Could not remove duplicate optional file: %1").arg(other);
            return false;
        }
    }
    file.completed = true;
    m_pending.markCompleted(file.key);
    return saveProgress();
}

void ModrinthPackInstallTask::executeTask()
{
    if (!prepare()) {
        finish();
        return;
    }
    for (auto& file : m_files) {
        if (file.unsupported) {
            file.completed = true;
            m_pending.markCompleted(file.key);
            continue;
        }
        const auto path = FS::PathCombine(m_instance->gameRoot(), file.path());
        // Updates retain their old difference-based download behavior.
        if (!m_isUpdate) {
            bool valid = matchesHash(path, file.hash);
            if (file.optional && !valid) {
                const auto otherPath = FS::PathCombine(m_instance->gameRoot(), file.alternatePath());
                if (matchesHash(otherPath, file.hash)) {
                    if ((QFile::exists(path) && !QFile::remove(path)) || !QFile::rename(otherPath, path)) {
                        m_error = tr("Could not move optional file from %1 to %2.").arg(otherPath, path);
                        finish();
                        return;
                    }
                    valid = true;
                }
            }
            if (valid && !completeFile(file)) {
                finish();
                return;
            }
        }
    }
    if (!saveProgress()) {
        finish();
        return;
    }
    downloadNextRound();
}

void ModrinthPackInstallTask::downloadNextRound()
{
    if (m_aborted || !m_error.isEmpty()) {
        finish();
        return;
    }
    auto job = makeShared<DrainingNetJob>(tr("Mod Download Modrinth"), APPLICATION->network());
    const auto dependencies = m_pending.remaining()["dependencies"].toObject();
    QString loader;
    if (dependencies.contains("fabric-loader"))
        loader = ModPlatform::getModLoaderAsString(ModPlatform::ModLoaderType::Fabric);
    if (dependencies.contains("quilt-loader"))
        loader = ModPlatform::getModLoaderAsString(ModPlatform::ModLoaderType::Quilt);
    if (dependencies.contains("forge"))
        loader = ModPlatform::getModLoaderAsString(ModPlatform::ModLoaderType::Forge);
    if (dependencies.contains("neoforge"))
        loader = ModPlatform::getModLoaderAsString(ModPlatform::ModLoaderType::NeoForge);
    Net::ModrinthDownloadMeta meta{ .reason = m_isUpdate ? "update" : "modpack",
                                    .gameVersion = dependencies["minecraft"].toString(),
                                    .loader = loader };
    for (auto& file : m_files) {
        if (file.completed || file.downloads.isEmpty()) {
            continue;
        }
        const auto path = FS::PathCombine(m_instance->gameRoot(), file.path());
        auto request = Net::ApiDownload::makeFile(file.downloads.dequeue(), path, Net::Download::Option::NoOptions, meta);
        request->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha512, file.hash));
        connect(request.get(), &Task::succeeded, this, [this, &file] {
            if (!file.completed && m_error.isEmpty() && !completeFile(file)) {
                m_downloadJob->abort();
            }
        });
        job->addNetAction(request);
    }
    if (job->size() == 0) {
        finishDownloads();
        return;
    }
    // A queued continuation also keeps request signal stacks out of staging commit.
    connect(job.get(), &Task::finished, this, [this] {
        QTimer::singleShot(0, this, [this] {
            if (m_aborted || !m_error.isEmpty()) {
                finish();
                return;
            }
            bool hasFallback = false;
            for (const auto& file : m_files) {
                hasFallback |= !file.completed && !file.downloads.isEmpty();
            }
            if (hasFallback) {
                downloadNextRound();
            } else {
                finishDownloads();
            }
        });
    });
    propagateFromOther(job.get());
    m_downloadJob = job;
    setStatus(tr("Downloading mods..."));
    job->start();
}

void ModrinthPackInstallTask::finishDownloads()
{
    QHash<QString, Resource*> resources;
    for (const auto& file : m_files) {
        if (!file.completed) {
            m_error = tr("Could not download all modpack files.");
        } else if (!file.unsupported && file.path().startsWith("mods/")) {
            auto mod = std::make_unique<Mod>(FS::PathCombine(m_instance->gameRoot(), file.path()));
            ModDetails details;
            details.mod_id = mod->fileinfo().absoluteFilePath();
            mod->setDetails(details);
            resources[file.hash.toHex()] = mod.get();
            m_resources.push_back(std::move(mod));
        }
    }
    if (!m_error.isEmpty() || resources.isEmpty()) {
        finish();
        return;
    }
    auto metadata = makeShared<EnsureMetadataTask>(resources, QDir(FS::PathCombine(m_instance->modsRoot(), ".index")),
                                                   ModPlatform::ResourceProvider::MODRINTH);
    connect(metadata.get(), &Task::failed, this,
            [this](const QString& reason) { logWarning(tr("Could not load mod display metadata: %1").arg(reason)); });
    connect(metadata.get(), &Task::finished, this, [this] { QTimer::singleShot(0, this, &ModrinthPackInstallTask::finish); });
    propagateFromOther(metadata.get());
    m_metadataTask = metadata;
    metadata->start();
}

void ModrinthPackInstallTask::finish()
{
    if (!isRunning()) {
        return;
    }
    if (!m_error.isEmpty()) {
        emitFailed(m_error);
    } else if (m_aborted) {
        emitAborted();
    } else {
        // ModpackResumeTask owns marker deletion after successful content work.
        // Resume progress, including an empty files array, stays persisted until then.
        emitSucceeded();
    }
}
