// SPDX-License-Identifier: GPL-3.0-only
/*
 * Content installation extracted from FlameInstanceCreationTask.cpp.
 * Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 * Copyright 2013-2021 MultiMC Contributors
 * Portions licensed under Apache-2.0; see FlameInstanceCreationTask.cpp.
 */

#include "FlamePackInstallTask.h"

#include <QCryptographicHash>
#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QTemporaryDir>

#include "Application.h"
#include "FileSystem.h"
#include "MMCZip.h"
#include "archive/ArchiveReader.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/World.h"
#include "minecraft/mod/tasks/LocalResourceParse.h"
#include "minecraft/mod/tasks/LocalResourceUpdateTask.h"
#include "modplatform/flame/ContentDownloadJob.h"
#include "modplatform/flame/FileResolvingTask.h"
#include "net/ApiDownload.h"
#include "net/ChecksumValidator.h"
#include "tasks/ConcurrentTask.h"
#include "ui/dialogs/BlockedModsDialog.h"
#include "ui/pages/modplatform/OptionalModDialog.h"

namespace {
QString fileKey(const Flame::File& file)
{
    return QString::number(file.projectId) + ':' + QString::number(file.fileId);
}

bool safePath(const QString& root, const QString& path)
{
    const auto relative = QDir(root).relativeFilePath(path);
    if (QDir::isAbsolutePath(relative) || relative == ".." || relative.startsWith("../"))
        return false;
    auto current = QDir(root).absolutePath();
    if (QFileInfo(current).isSymLink())
        return false;
    for (const auto& part : relative.split('/')) {
        current = QDir(current).filePath(part);
        if (QFileInfo(current).isSymLink())
            return false;
    }
    return true;
}

bool safeTree(const QString& root)
{
    if (!safePath(root, root))
        return false;
    QDirIterator it(root, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (QFileInfo(it.next()).isSymLink())
            return false;
    }
    return true;
}

std::optional<QCryptographicHash::Algorithm> hashAlgorithm(const Flame::File& file)
{
    const auto hex = file.version.hash.toLatin1();
    if (QByteArray::fromHex(hex).toHex() != hex.toLower())
        return {};
    if (file.version.hash_type == "sha1" && file.version.hash.size() == 40)
        return QCryptographicHash::Sha1;
    if (file.version.hash_type == "md5" && file.version.hash.size() == 32)
        return QCryptographicHash::Md5;
    return {};
}

bool containsWorld(const QString& path)
{
    // Inspect the archive directly: World only recognizes an on-disk .zip
    // suffix, whereas optional downloads can be named .zip.disabled.
    MMCZip::ArchiveReader archive(path);
    bool found = false;
    return archive.parse([&](MMCZip::ArchiveReader::File* entry, bool& stop) {
        found = entry->isFile() && QFileInfo(entry->filename()).fileName().compare("level.dat", Qt::CaseInsensitive) == 0;
        stop = found;
        return true;
    }) && found;
}
}  // namespace

FlamePackInstallTask::FlamePackInstallTask(MinecraftInstance* instance, QJsonObject manifest, QWidget* parent)
    : m_instance(instance), m_manifest(std::move(manifest)), m_parent(parent)
{}

bool FlamePackInstallTask::abort()
{
    m_abort = true;
    setStatus(tr("Stopping installation; waiting for active requests to finish..."));
    if (m_dialog)
        m_dialog->reject();
    if (m_filesJob)
        m_filesJob->abort();
    // FileResolvingTask has the same early-abort issue. Let its API requests
    // settle; executeTask() observes the cancellation before writing any files.
    return true;
}

void FlamePackInstallTask::fail(const QString& reason)
{
    if (m_error.isEmpty())
        m_error = reason;
}

bool FlamePackInstallTask::checkPath(const QString& path)
{
    if (safePath(m_instance->instanceRoot(), path))
        return true;
    fail(tr("Unsafe modpack path: %1").arg(path));
    return false;
}

bool FlamePackInstallTask::runTask(const Task::Ptr& task)
{
    QEventLoop loop;
    connect(task.get(), &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    propagateFromOther(task.get());
    task->start();
    if (!task->isFinished())
        loop.exec();
    return task->wasSuccessful();
}

bool FlamePackInstallTask::validFile(const Flame::File& file, const QString& path) const
{
    auto algorithm = hashAlgorithm(file);
    if (!algorithm || !safePath(m_instance->instanceRoot(), path) || !QFileInfo(path).isFile())
        return false;
    QFile input(path);
    QCryptographicHash hash(*algorithm);
    return input.open(QIODevice::ReadOnly) && hash.addData(&input) &&
           hash.result().toHex().compare(file.version.hash.toLatin1(), Qt::CaseInsensitive) == 0;
}

bool FlamePackInstallTask::installCachedFile(const Flame::File& file, const MetaEntryPtr& entry, const QString& path, bool disabled)
{
    const auto algorithm = *hashAlgorithm(file);
    const auto expected = QByteArray::fromHex(file.version.hash.toLatin1());
    if (!Modpack::Cache::matchesHash(entry->getFullPath(), algorithm, expected)) {
        Modpack::Cache::invalidate(entry);
        fail(tr("Cached file is missing or failed hash validation: %1").arg(file.version.fileName));
        return false;
    }
    QString error;
    if (!checkPath(path) || !Modpack::Cache::copyVerified(entry->getFullPath(), path, algorithm, expected, &error)) {
        fail(tr("Could not install cached resource: %1").arg(error));
        return false;
    }
    return completeFile(file, path, disabled);
}

bool FlamePackInstallTask::installWorld(const QString& path, bool disabled, QString* installedPath)
{
    const auto saves = QDir(m_instance->gameRoot()).filePath("saves");
    if (!checkPath(saves) || !FS::ensureFolderPathExists(saves)) {
        fail(tr("Could not create the world saves directory: %1").arg(saves));
        return false;
    }
    QTemporaryDir temporary(QDir(saves).filePath(".flame-world-XXXXXX"));
    const auto extracted = QDir(temporary.path()).filePath("world");
    if (!temporary.isValid() || !checkPath(extracted)) {
        fail(tr("Could not create a temporary directory for world installation."));
        return false;
    }

    // World::install() does not reliably propagate extraction failure. Use the
    // same extractor, checking its result and every archive path before writing.
    MMCZip::ArchiveReader archive(path);
    QString prefix;
    bool foundLevel = false;
    if (!archive.parse([&](MMCZip::ArchiveReader::File* entry) {
            const auto name = entry->filename();
            if (QDir::isAbsolutePath(name) || name.contains('\\') || name.split('/').contains("..") ||
                !checkPath(QDir(extracted).filePath(name))) {
                fail(tr("Unsafe world archive path: %1").arg(name));
                return false;
            }
            if (!foundLevel && entry->isFile() && QFileInfo(name).fileName().compare("level.dat", Qt::CaseInsensitive) == 0) {
                prefix = name.chopped(QString("level.dat").size());
                foundLevel = true;
            }
            return true;
        }) ||
        !foundLevel || !FS::ensureFolderPathExists(extracted) || !MMCZip::extractSubDir(&archive, prefix, extracted).has_value()) {
        fail(tr("Could not install world: %1").arg(path));
        return false;
    }
    if (!safeTree(extracted)) {
        fail(tr("The installed resource contains an unsafe symbolic link."));
        return false;
    }
    World world{ QFileInfo(extracted) };
    if (!world.isValid()) {
        fail(tr("The extracted world is not valid: %1").arg(path));
        return false;
    }
    // An unselected optional world remains a verified disabled archive. The
    // temporary extracted copy is only for validation and is removed on return.
    if (disabled)
        return true;
    const auto folder = FS::DirNameFromString(world.name(), saves);
    const auto target = QDir(saves).filePath(folder);
    if (folder.isEmpty() || !checkPath(target) || !QDir(saves).mkdir(folder)) {
        fail(tr("Could not install world: %1").arg(path));
        return false;
    }
    // Successful mkdir reserves a fresh directory. Roll back only this directory
    // if moving the extracted world fails.
    *installedPath = target;
    if (!FS::move(extracted, target)) {
        fail(tr("Could not install world: %1").arg(path));
        rollbackWorld(target);
        installedPath->clear();
        return false;
    }
    return true;
}

void FlamePackInstallTask::rollbackWorld(const QString& path)
{
    if (path.isEmpty())
        return;
    if (!checkPath(path) || !FS::deletePath(path)) {
        const auto reason = tr("Could not remove the uncommitted world directory: %1").arg(path);
        logWarning(reason);
        fail(reason);
    }
}

bool FlamePackInstallTask::completeFile(const Flame::File& file, QString path, bool disabled)
{
    const auto key = fileKey(file);
    if (m_completed.contains(key))
        return true;
    if (!validFile(file, path)) {
        fail(tr("File is missing or failed hash validation: %1").arg(path));
        return false;
    }
    const auto alternate = disabled ? path.chopped(9) : path + ".disabled";
    if (!checkPath(alternate))
        return false;
    if (QFile::exists(alternate) && !QFile::remove(alternate)) {
        fail(tr("Could not remove duplicate resource: %1").arg(alternate));
        return false;
    }

    auto type = file.resourceType;
    if (type != ModPlatform::ResourceType::Mod)
        type = ResourceUtils::identify(QFileInfo(path));
    // A project explicitly classified as a world must never be marked complete
    // merely because resource identification failed.
    if (file.resourceType == ModPlatform::ResourceType::World)
        type = ModPlatform::ResourceType::World;
    if (type == ModPlatform::ResourceType::Unknown && file.version.fileName.endsWith(".zip") && containsWorld(path))
        type = ModPlatform::ResourceType::World;
    QString folder;
    switch (type) {
        case ModPlatform::ResourceType::Mod:
            folder = "mods";
            break;
        case ModPlatform::ResourceType::ResourcePack:
            folder = "resourcepacks";
            break;
        case ModPlatform::ResourceType::TexturePack:
            folder = "texturepacks";
            break;
        case ModPlatform::ResourceType::DataPack:
            folder = "datapacks";
            break;
        case ModPlatform::ResourceType::ShaderPack:
            folder = "shaderpacks";
            break;
        case ModPlatform::ResourceType::World:
            folder = "saves";
            break;
        default:
            break;
    }
    if (!folder.isEmpty()) {
        const auto destination = FS::PathCombine(m_instance->gameRoot(), folder, QFileInfo(path).fileName());
        if (!checkPath(destination))
            return false;
        if (path != destination) {
            if (!FS::ensureFilePathExists(destination) || !FS::move(path, destination)) {
                fail(tr("Could not move resource to %1").arg(destination));
                return false;
            }
            path = destination;
        }
        const auto otherState = disabled ? path.chopped(9) : path + ".disabled";
        if (!checkPath(otherState))
            return false;
        if (QFile::exists(otherState) && !QFile::remove(otherState)) {
            fail(tr("Could not remove duplicate resource: %1").arg(otherState));
            return false;
        }
    }
    QString installedWorld;
    if (type == ModPlatform::ResourceType::World && !installWorld(path, disabled, &installedWorld))
        return false;
    if (type == ModPlatform::ResourceType::World && !safeTree(m_instance->instanceRoot())) {
        fail(tr("The installed resource contains an unsafe symbolic link."));
        rollbackWorld(installedWorld);
        return false;
    }

    m_completed.insert(key);
    if (type == ModPlatform::ResourceType::Mod)
        m_metadataFiles.append(file);
    return true;
}

void FlamePackInstallTask::executeTask()
{
    Flame::Manifest manifest;
    if (m_manifest.value("manifestType").toString() != "minecraftModpack" || m_manifest.value("manifestVersion").toInt() != 1 ||
        !m_manifest.value("files").isArray() || !safeTree(m_instance->instanceRoot()) || !checkPath(m_instance->gameRoot())) {
        emitFailed(tr("Invalid or unsafe CurseForge manifest."));
        return;
    }
    for (const auto& value : m_manifest.value("files").toArray()) {
        const auto entry = value.toObject();
        Flame::File file;
        file.projectId = entry.value("projectID").toInt();
        file.fileId = entry.value("fileID").toInt();
        file.required = entry.value("required").toBool(true);
        file.resourceType = ModPlatform::ResourceType::Unknown;
        if (file.projectId <= 0 || file.fileId <= 0 || manifest.files.contains(file.fileId)) {
            emitFailed(tr("Invalid or duplicate CurseForge file ID."));
            return;
        }
        manifest.files.insert(file.fileId, file);
    }
    installFiles(manifest);
}

void FlamePackInstallTask::installFiles(const Flame::Manifest& manifest)
{
    if (m_abort) {
        emitAborted();
        return;
    }
    auto resolver = makeShared<Flame::FileResolvingTask>(manifest);
    const auto resolved = runTask(resolver);
    const auto results = resolver->getResults().files;
    if (!resolved) {
        if (resolver->filesResolved())
            logWarning(tr("Could not load project metadata: %1").arg(resolver->failReason()));
        else
            fail(tr("Unable to resolve mod IDs:\n%1").arg(resolver->failReason()));
    }
    if (m_error.isEmpty() && resolver->filesResolved()) {
        QStringList optionalFiles;
        QSet<QString> destinations;
        for (const auto& file : results) {
            const auto name = file.version.fileName;
            if (name.isEmpty() || name == "." || name == ".." || name.contains('/') || name.contains('\\') ||
                name != FS::RemoveInvalidPathChars(name) || !hashAlgorithm(file)) {
                fail(tr("Invalid file information returned for CurseForge file %1.").arg(file.fileId));
                break;
            }
            const auto destination = FS::PathCombine(file.targetFolder, name);
            if (destinations.contains(destination) || destinations.contains(destination + ".disabled")) {
                fail(tr("Multiple CurseForge files have the same destination: %1").arg(destination));
                break;
            }
            destinations.insert(destination);
            destinations.insert(destination + ".disabled");
            if (!checkPath(FS::PathCombine(m_instance->gameRoot(), file.targetFolder, name)) ||
                !checkPath(FS::PathCombine(m_instance->gameRoot(), file.targetFolder, name + ".disabled")))
                break;
            if (!file.required)
                optionalFiles.append(FS::PathCombine(file.targetFolder, name));
        }
        QStringList selected;
        if (m_error.isEmpty() && !m_abort && !optionalFiles.isEmpty()) {
            OptionalModDialog dialog(m_parent, optionalFiles);
            m_dialog = &dialog;
            if (dialog.exec() == QDialog::Rejected)
                m_abort = true;
            selected = dialog.getResult();
            m_dialog = nullptr;
        }

        QMap<QString, QList<BlockedMod>> blocked;
        QMap<QString, Flame::File> blockedFiles;
        QMap<QString, MetaEntryPtr> cacheEntries;
        m_filesJob = makeShared<Flame::ContentDownloadJob>(tr("Mod Download Flame"), APPLICATION->network());
        for (const auto& file : results) {
            if (m_abort || !m_error.isEmpty())
                break;
            const auto relative = FS::PathCombine(file.targetFolder, file.version.fileName);
            const auto disabled = !file.required && !selected.contains(relative);
            const auto path = QDir(m_instance->gameRoot()).filePath(relative) + (disabled ? ".disabled" : "");
            const auto algorithm = *hashAlgorithm(file);
            const auto expected = QByteArray::fromHex(file.version.hash.toLatin1());
            auto entry = Modpack::Cache::resolveEntry(*APPLICATION->metacache(), Modpack::Cache::Platform::CurseForge, algorithm, expected);
            if (!entry) {
                fail(tr("Invalid cache information for CurseForge file %1.").arg(file.fileId));
                break;
            }
            cacheEntries.insert(fileKey(file), entry);
            // Check the cache before asking for a restricted file again.
            if (!entry->isStale()) {
                if (!installCachedFile(file, entry, path, disabled))
                    break;
                continue;
            }
            if (file.version.downloadUrl.isEmpty()) {
                BlockedMod mod;
                mod.name = file.version.fileName;
                mod.websiteUrl = QString("%1/download/%2").arg(file.pack.websiteUrl, QString::number(file.fileId));
                mod.hash = file.version.hash;
                mod.matched = false;
                mod.targetFolder = file.targetFolder;
                mod.disabled = disabled;
                blocked[file.version.hash_type].append(mod);
                blockedFiles.insert(FS::PathCombine(mod.targetFolder, mod.name), file);
                continue;
            }
            auto download = Net::ApiDownload::makeCached(file.version.downloadUrl, entry, Net::Download::Option::MakeEternal);
            download->addValidator(new Net::ChecksumValidator(algorithm, expected));
            connect(
                download.get(), &Task::succeeded, this,
                [this, file, entry, path, disabled] {
                    if (m_abort || !m_error.isEmpty())
                        return;
                    if (!installCachedFile(file, entry, path, disabled) && m_filesJob)
                        m_filesJob->abort();
                },
                Qt::QueuedConnection);
            m_filesJob->addNetAction(download);
        }
        for (auto group = blocked.begin(); group != blocked.end() && !m_abort && m_error.isEmpty(); ++group) {
            BlockedModsDialog dialog(m_parent, tr("Blocked mods found"),
                                     tr("The following files must be downloaded manually before installation can finish."), group.value(),
                                     group.key());
            m_dialog = &dialog;
            if (dialog.exec() != QDialog::Accepted)
                m_abort = true;
            m_dialog = nullptr;
            for (const auto& mod : group.value()) {
                if (m_abort || !m_error.isEmpty())
                    break;
                const auto file = blockedFiles.value(FS::PathCombine(mod.targetFolder, mod.name));
                const auto entry = cacheEntries.value(fileKey(file));
                const auto path = FS::PathCombine(m_instance->gameRoot(), mod.targetFolder, mod.name) + (mod.disabled ? ".disabled" : "");
                QString error;
                if (!mod.matched ||
                    !Modpack::Cache::storeFile(*APPLICATION->metacache(), entry, mod.localPath, *hashAlgorithm(file),
                                               QByteArray::fromHex(file.version.hash.toLatin1()), &error) ||
                    !installCachedFile(file, entry, path, mod.disabled)) {
                    fail(tr("Could not install manually downloaded file: %1\n%2").arg(mod.name, error));
                    break;
                }
                if (mod.move && QFileInfo(mod.localPath).absoluteFilePath() != QFileInfo(entry->getFullPath()).absoluteFilePath() &&
                    !QFile::remove(mod.localPath)) {
                    logWarning(tr("The file was installed, but its original copy could not be removed: %1").arg(mod.localPath));
                }
            }
        }
        if (!m_abort && m_error.isEmpty()) {
            const auto succeeded = runTask(m_filesJob);
            if (!succeeded && !m_abort)
                fail(m_filesJob->failReason());
        }
        m_filesJob.reset();
    }

    // Display metadata must not turn already installed content into a failed download.
    if (!m_abort && m_error.isEmpty() && !m_metadataFiles.isEmpty()) {
        auto metadata = makeShared<ConcurrentTask>("CreateModMetadata", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt());
        const auto folder = FS::PathCombine(m_instance->gameRoot(), "mods", ".index");
        if (checkPath(folder) && FS::ensureFolderPathExists(folder)) {
            for (auto file : m_metadataFiles) {
                if (file.pack.addonId == QString::number(file.projectId))
                    metadata->addTask(makeShared<LocalResourceUpdateTask>(folder, file.pack, file.version));
            }
            if (!runTask(metadata))
                logWarning(tr("Could not save mod metadata: %1").arg(metadata->failReason()));
        } else {
            logWarning(tr("Could not create the mod metadata directory."));
        }
    }
    if (m_error.isEmpty() && !m_abort && m_completed.size() != manifest.files.size())
        fail(tr("Some modpack files have not been installed."));
    if (!m_error.isEmpty())
        emitFailed(m_error);
    else if (m_abort)
        emitAborted();
    else
        emitSucceeded();
}
