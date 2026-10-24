// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "FileResolvingTask.h"
#include <algorithm>

#include "FileSystem.h"
#include "Json.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/modrinth/ModrinthAPI.h"

#include "modplatform/modrinth/ModrinthPackIndex.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

#include "Application.h"

static const FlameAPI flameAPI;
static ModrinthAPI modrinthAPI;

Flame::FileResolvingTask::FileResolvingTask(Flame::Manifest& toProcess) : m_manifest(toProcess) {}

bool Flame::FileResolvingTask::abort()
{
    bool aborted = true;
    if (m_task) {
        aborted = m_task->abort();
    }
    return aborted ? Task::abort() : false;
}

void Flame::FileResolvingTask::executeTask()
{
    m_filesResolved = false;
    m_downloadFailed = false;
    if (m_manifest.files.isEmpty()) {  // no file to resolve so leave it empty and emit success immediately
        m_filesResolved = true;
        emitSucceeded();
        return;
    }
    setStatus(tr("Resolving mod IDs..."));
    setProgress(0, 3);

    QStringList fileIds;
    for (auto file : m_manifest.files) {
        fileIds.push_back(QString::number(file.fileId));
    }
    auto [task, response] = flameAPI.getFiles(fileIds);
    m_task = task;

    auto step_progress = std::make_shared<TaskStepProgress>();
    connect(m_task.get(), &Task::succeeded, this, [this, response, step_progress]() {
        step_progress->state = TaskStepState::Succeeded;
        stepProgress(*step_progress);
        netJobFinished(response);
    });
    connect(m_task.get(), &Task::failed, this, [this, step_progress](QString reason) {
        m_downloadFailed = true;
        step_progress->state = TaskStepState::Failed;
        stepProgress(*step_progress);
        emitFailed(reason);
    });
    connect(m_task.get(), &Task::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_task.get(), &Task::progress, this, [this, step_progress](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        step_progress->update(current, total);
        stepProgress(*step_progress);
    });
    connect(m_task.get(), &Task::status, this, [this, step_progress](QString status) {
        step_progress->status = status;
        stepProgress(*step_progress);
    });

    m_task->start();
}

ModPlatform::ResourceType getResourceType(int classId)
{
    switch (classId) {
        case 17:  // Worlds
            return ModPlatform::ResourceType::World;
        case 6:  // Mods
            return ModPlatform::ResourceType::Mod;
        case 12:  // Resource Packs
                  // return ModPlatform::ResourceType::ResourcePack; // not really a resourcepack
            /* fallthrough */
        case 4546:  // Customization
                    // return ModPlatform::ResourceType::ShaderPack; // not really a shaderPack
            /* fallthrough */
        case 4471:  // Modpacks
            /* fallthrough */
        case 5:  // Bukkit Plugins
            /* fallthrough */
        case 4559:  // Addons
            /* fallthrough */
        default:
            return ModPlatform::ResourceType::Unknown;
    }
}

void Flame::FileResolvingTask::netJobFinished(QByteArray* response)
{
    setProgress(1, 3);
    // job to check modrinth for blocked projects
    QJsonDocument doc;
    QJsonArray array;

    try {
        doc = Json::requireDocument(*response);
        array = Json::requireArray(doc.object()["data"]);
    } catch (Json::JsonException& e) {
        qCritical() << "Non-JSON data returned from the CF API";
        qCritical() << e.cause();

        emitFailed(tr("Invalid file data returned by the CurseForge API: %1").arg(e.cause()));

        return;
    }

    QStringList hashes;
    QMap<int, ModPlatform::IndexedVersion> versions;
    for (QJsonValueRef file : array) {
        try {
            auto obj = Json::requireObject(file);
            const auto fileid = Json::requireInteger(obj, "id");
            const auto projectid = Json::requireInteger(obj, "modId");
            if (!m_manifest.files.contains(fileid) || versions.contains(fileid) || m_manifest.files[fileid].projectId != projectid)
                throw Json::JsonException(tr("Unexpected or duplicate file ID %1 returned by the API.").arg(fileid));
            const auto name = Json::requireString(obj, "fileName");
            if (name.isEmpty() || name == "." || name == ".." || name.contains('/') || name.contains('\\') ||
                name != FS::RemoveInvalidPathChars(name))
                throw Json::JsonException(tr("Invalid filename returned by the API: %1").arg(name));
            auto version = FlameMod::loadIndexedPackVersion(obj);
            const auto hash = version.hash.toLatin1();
            if (!((version.hash_type == "sha1" && hash.size() == 40) || (version.hash_type == "md5" && hash.size() == 32)) ||
                QByteArray::fromHex(hash).toHex() != hash.toLower())
                throw Json::JsonException(tr("Invalid file hash returned by the API for file %1.").arg(fileid));
            versions.insert(fileid, version);
            auto url = QUrl(version.downloadUrl, QUrl::TolerantMode);
            if (!url.isValid() && "sha1" == version.hash_type && !version.hash.isEmpty()) {
                hashes.push_back(version.hash);
            }
        } catch (Json::JsonException& e) {
            qCritical() << "Non-JSON data returned from the CF API";
            qCritical() << e.cause();

            emitFailed(tr("Invalid file data returned by the CurseForge API: %1").arg(e.cause()));

            return;
        }
    }
    if (versions.size() != m_manifest.files.size()) {
        emitFailed(tr("The API did not return information for all requested files."));
        return;
    }
    for (auto file = versions.constBegin(); file != versions.constEnd(); ++file)
        m_manifest.files[file.key()].version = file.value();
    // Only a completely validated file response may become a metadata-only
    // failure later. Never infer this from partially populated file fields.
    m_filesResolved = true;
    if (hashes.isEmpty()) {
        getFlameProjects();
        return;
    }
    auto [modrinthTask, modrinthResponse] = modrinthAPI.currentVersions(hashes, "sha1");
    m_task = modrinthTask;
    (dynamic_cast<NetJob*>(m_task.get()))->setAskRetry(false);
    auto step_progress = std::make_shared<TaskStepProgress>();
    connect(m_task.get(), &Task::succeeded, this, [this, modrinthResponse, step_progress]() {
        step_progress->state = TaskStepState::Succeeded;
        stepProgress(*step_progress);
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*modrinthResponse, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth::CurrentVersions at" << parse_error.offset
                       << "reason:" << parse_error.errorString();
            qWarning() << *modrinthResponse;

            getFlameProjects();
            return;
        }
        if (APPLICATION->settings()->get("FallbackMRBlockedMods").toBool()) {
            try {
                auto entries = Json::requireObject(doc);
                for (auto& out : m_manifest.files) {
                    auto url = QUrl(out.version.downloadUrl, QUrl::TolerantMode);
                    if (!url.isValid() && "sha1" == out.version.hash_type && !out.version.hash.isEmpty()) {
                        try {
                            auto entry = Json::requireObject(entries, out.version.hash);

                            auto file = Modrinth::loadIndexedPackVersion(entry);

                            out.version.downloadUrl = file.downloadUrl;
                            qDebug() << "Found alternative on modrinth" << out.version.fileName;
                        } catch (Json::JsonException& e) {
                            qDebug() << e.cause();
                            qDebug() << entries;
                        }
                    }
                }
            } catch (Json::JsonException& e) {
                qDebug() << e.cause();
                qDebug() << doc;
            }
        }
        getFlameProjects();
    });
    connect(m_task.get(), &Task::failed, this, [this, step_progress](QString reason) {
        step_progress->state = TaskStepState::Failed;
        stepProgress(*step_progress);
        getFlameProjects();
    });
    connect(m_task.get(), &Task::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_task.get(), &Task::progress, this, [this, step_progress](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        step_progress->update(current, total);
        stepProgress(*step_progress);
    });
    connect(m_task.get(), &Task::status, this, [this, step_progress](QString status) {
        step_progress->status = status;
        stepProgress(*step_progress);
    });
    m_task->start();
}

void Flame::FileResolvingTask::getFlameProjects()
{
    setProgress(2, 3);
    QStringList addonIds;
    for (auto file : m_manifest.files) {
        addonIds.push_back(QString::number(file.projectId));
    }

    auto [task, response] = flameAPI.getProjects(addonIds);
    m_task = task;

    auto step_progress = std::make_shared<TaskStepProgress>();
    connect(m_task.get(), &Task::succeeded, this, [this, response, step_progress] {
        QJsonParseError parse_error{};
        auto doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth projects task at" << parse_error.offset
                       << "reason:" << parse_error.errorString();
            qWarning() << *response;
            step_progress->state = TaskStepState::Failed;
            stepProgress(*step_progress);
            emitFailed(tr("Could not parse the CurseForge project response at offset %1: %2")
                           .arg(parse_error.offset)
                           .arg(parse_error.errorString()));
            return;
        }

        try {
            QJsonArray entries;
            entries = Json::requireArray(Json::requireObject(doc), "data");

            for (auto entry : entries) {
                auto entry_obj = Json::requireObject(entry);
                auto id = Json::requireInteger(entry_obj, "id");
                auto file = std::find_if(m_manifest.files.begin(), m_manifest.files.end(),
                                         [id](const Flame::File& file) { return file.projectId == id; });
                if (file == m_manifest.files.end()) {
                    continue;
                }

                setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(file->version.fileName));
                FlameMod::loadIndexedPack(file->pack, entry_obj);
                file->resourceType = getResourceType(Json::requireInteger(entry_obj, "classId", "modClassId"));
                if (file->resourceType == ModPlatform::ResourceType::World) {
                    file->targetFolder = "saves";
                }
            }
        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
            step_progress->state = TaskStepState::Failed;
            stepProgress(*step_progress);
            emitFailed(tr("Invalid project data returned by the CurseForge API: %1").arg(e.cause()));
            return;
        }
        step_progress->state = TaskStepState::Succeeded;
        stepProgress(*step_progress);
        emitSucceeded();
    });

    connect(m_task.get(), &Task::failed, this, [this, step_progress](QString reason) {
        step_progress->state = TaskStepState::Failed;
        stepProgress(*step_progress);
        emitFailed(reason);
    });
    connect(m_task.get(), &Task::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_task.get(), &Task::progress, this, [this, step_progress](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        step_progress->update(current, total);
        stepProgress(*step_progress);
    });
    connect(m_task.get(), &Task::status, this, [this, step_progress](QString status) {
        step_progress->status = status;
        stepProgress(*step_progress);
    });

    m_task->start();
}
