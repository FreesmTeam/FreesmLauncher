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
#include "modplatform/modrinth/ModrinthDownload.h"
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
}  // namespace

ModrinthPackInstallTask::ModrinthPackInstallTask(MinecraftInstance* instance, QJsonObject manifest, bool trustedSource, QWidget* parent)
    : m_instance(instance), m_manifest(std::move(manifest)), m_trustedSource(trustedSource), m_parent(parent)
{
    setAbortable(true);
}

ModrinthPackInstallTask::~ModrinthPackInstallTask() = default;

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
    // a user-edited directory may contain a symlink to outside the instance.
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
    const auto& manifest = m_manifest;
    QStringList optionalPaths;
    QStringList untrusted;
    QSet<QString> targets;
    try {
        if (!validatePath("mods/.index")) {
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
    return true;
}

bool ModrinthPackInstallTask::completeFile(File& file)
{
    // A successful HTTP request (for example, 204) need not have committed a file.
    const auto cachePath = file.cacheEntry->getFullPath();
    if (!Modpack::Cache::matchesHash(cachePath, QCryptographicHash::Sha512, file.hash)) {
        Modpack::Cache::invalidate(file.cacheEntry);
        return false;
    }
    QString error;
    const auto target = FS::PathCombine(m_instance->gameRoot(), file.path());
    if (!validatePath(file.path()) || !Modpack::Cache::copyVerified(cachePath, target, QCryptographicHash::Sha512, file.hash, &error)) {
        m_error = tr("Could not install cached file %1: %2").arg(file.path(), error);
        return false;
    }

    // Optional selection changes must not leave enabled and disabled duplicates.
    if (file.optional) {
        const auto other = FS::PathCombine(m_instance->gameRoot(), file.alternatePath());
        if (QFile::exists(other) && !QFile::remove(other)) {
            m_error = tr("Could not remove duplicate optional file: %1").arg(other);
            return false;
        }
    }
    file.completed = true;
    return true;
}

void ModrinthPackInstallTask::executeTask()
{
    if (!prepare()) {
        finish();
        return;
    }
    installFiles();
}

void ModrinthPackInstallTask::installFiles()
{
    if (m_aborted) {
        finish();
        return;
    }
    for (auto& file : m_files) {
        if (file.unsupported) {
            file.completed = true;
            continue;
        }
        file.cacheEntry = Modpack::Cache::resolveEntry(*APPLICATION->metacache(), Modpack::Cache::Platform::Modrinth,
                                                       QCryptographicHash::Sha512, file.hash);
        if (!file.cacheEntry) {
            m_error = tr("Invalid cache information for %1.").arg(file.key);
            finish();
            return;
        }
        if (!file.cacheEntry->isStale() && !completeFile(file) && !m_error.isEmpty()) {
            finish();
            return;
        }
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
    const auto dependencies = m_manifest["dependencies"].toObject();
    QString loader;
    if (dependencies.contains("fabric-loader"))
        loader = ModPlatform::getModLoaderAsString(ModPlatform::ModLoaderType::Fabric);
    if (dependencies.contains("quilt-loader"))
        loader = ModPlatform::getModLoaderAsString(ModPlatform::ModLoaderType::Quilt);
    if (dependencies.contains("forge"))
        loader = ModPlatform::getModLoaderAsString(ModPlatform::ModLoaderType::Forge);
    if (dependencies.contains("neoforge"))
        loader = ModPlatform::getModLoaderAsString(ModPlatform::ModLoaderType::NeoForge);
    Modrinth::DownloadMeta meta{ .reason = m_isUpdate ? "update" : "modpack",
                                 .gameVersion = dependencies["minecraft"].toString(),
                                 .loader = loader };
    for (auto& file : m_files) {
        if (file.completed || file.downloads.isEmpty()) {
            continue;
        }
        auto request = Net::ApiDownload::makeCached(file.downloads.dequeue(), file.cacheEntry, Net::Download::Option::MakeEternal);
        request->addHeaderProxy(std::make_unique<Modrinth::DownloadHeaderProxy>(meta));
        request->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha512, file.hash));
        connect(request.get(), &Task::succeeded, this, [this, &file] {
            if (m_aborted || file.completed || !m_error.isEmpty()) {
                return;
            }
            if (!completeFile(file) && !m_error.isEmpty()) {
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
        emitSucceeded();
    }
}
