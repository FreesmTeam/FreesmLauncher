// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Freesm Launcher - Minecraft Launcher
 *  Copyright (C) 2026 FreesmModders
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

#include "ModpackChangelogGenerator.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <algorithm>

#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"

// ─── ModSnapshotEntry ───────────────────────────────────────────────────────

QJsonObject ModSnapshotEntry::toJson() const
{
    QJsonObject obj;
    obj["mod_id"] = modId;
    obj["name"] = name;
    obj["version"] = version;
    obj["file_name"] = fileName;
    return obj;
}

ModSnapshotEntry ModSnapshotEntry::fromJson(const QJsonObject& obj)
{
    ModSnapshotEntry entry;
    entry.modId = obj["mod_id"].toString();
    entry.name = obj["name"].toString();
    entry.version = obj["version"].toString();
    entry.fileName = obj["file_name"].toString();
    return entry;
}

// ─── ModpackVersionSnapshot ────────────────────────────────────────────────

QJsonObject ModpackVersionSnapshot::toJson() const
{
    QJsonObject obj;
    obj["version_number"] = versionNumber;
    obj["version_label"] = versionLabel;
    obj["timestamp"] = timestamp.toString(Qt::ISODate);

    QJsonArray modsArray;
    for (const auto& mod : mods) {
        modsArray.append(mod.toJson());
    }
    obj["mods"] = modsArray;
    return obj;
}

ModpackVersionSnapshot ModpackVersionSnapshot::fromJson(const QJsonObject& obj)
{
    ModpackVersionSnapshot snapshot;
    snapshot.versionNumber = obj["version_number"].toInt();
    snapshot.versionLabel = obj["version_label"].toString();
    snapshot.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);

    QJsonArray modsArray = obj["mods"].toArray();
    for (const auto& modVal : modsArray) {
        snapshot.mods.append(ModSnapshotEntry::fromJson(modVal.toObject()));
    }
    return snapshot;
}

// ─── ModpackChangelog ──────────────────────────────────────────────────────

bool ModpackChangelog::isEmpty() const
{
    return addedMods.isEmpty() && removedMods.isEmpty() && updatedMods.isEmpty();
}

QString ModpackChangelog::formatAsText() const
{
    QString result;

    if (!updatedMods.isEmpty()) {
        result += "**Updated**\n";
        // Sort alphabetically by mod name
        auto sorted = updatedMods;
        std::sort(sorted.begin(), sorted.end(), [](const ChangelogEntry& a, const ChangelogEntry& b) {
            return a.modName.toLower() < b.modName.toLower();
        });
        for (const auto& entry : sorted) {
            result += "- " + entry.modName + "\n";
        }
        result += "\n";
    }

    if (!addedMods.isEmpty()) {
        result += "**Added**\n";
        auto sorted = addedMods;
        std::sort(sorted.begin(), sorted.end(), [](const ChangelogEntry& a, const ChangelogEntry& b) {
            return a.modName.toLower() < b.modName.toLower();
        });
        for (const auto& entry : sorted) {
            result += "- " + entry.modName + "\n";
        }
        result += "\n";
    }

    if (!removedMods.isEmpty()) {
        result += "**Removed**\n";
        auto sorted = removedMods;
        std::sort(sorted.begin(), sorted.end(), [](const ChangelogEntry& a, const ChangelogEntry& b) {
            return a.modName.toLower() < b.modName.toLower();
        });
        for (const auto& entry : sorted) {
            result += "- " + entry.modName + "\n";
        }
        result += "\n";
    }

    return result.trimmed();
}

QString ModpackChangelog::formatAsMarkdown() const
{
    QString result;
    result += QString("## Changelog (v%1 → v%2)\n\n").arg(fromVersion).arg(toVersion);

    if (!updatedMods.isEmpty()) {
        result += "### Updated\n";
        auto sorted = updatedMods;
        std::sort(sorted.begin(), sorted.end(), [](const ChangelogEntry& a, const ChangelogEntry& b) {
            return a.modName.toLower() < b.modName.toLower();
        });
        for (const auto& entry : sorted) {
            if (!entry.oldVersion.isEmpty() && !entry.newVersion.isEmpty()) {
                result += QString("- **%1** (%2 → %3)\n").arg(entry.modName, entry.oldVersion, entry.newVersion);
            } else {
                result += "- **" + entry.modName + "**\n";
            }
        }
        result += "\n";
    }

    if (!addedMods.isEmpty()) {
        result += "### Added\n";
        auto sorted = addedMods;
        std::sort(sorted.begin(), sorted.end(), [](const ChangelogEntry& a, const ChangelogEntry& b) {
            return a.modName.toLower() < b.modName.toLower();
        });
        for (const auto& entry : sorted) {
            result += "- " + entry.modName + "\n";
        }
        result += "\n";
    }

    if (!removedMods.isEmpty()) {
        result += "### Removed\n";
        auto sorted = removedMods;
        std::sort(sorted.begin(), sorted.end(), [](const ChangelogEntry& a, const ChangelogEntry& b) {
            return a.modName.toLower() < b.modName.toLower();
        });
        for (const auto& entry : sorted) {
            result += "- ~~" + entry.modName + "~~\n";
        }
        result += "\n";
    }

    return result.trimmed();
}

// ─── ModpackChangelogGenerator ─────────────────────────────────────────────

ModpackChangelogGenerator::ModpackChangelogGenerator(MinecraftInstance* instance) : m_instance(instance) {}

QString ModpackChangelogGenerator::historyFilePath() const
{
    return QDir(m_instance->instanceRoot()).filePath("modpack_version_history.json");
}

QList<ModpackVersionSnapshot> ModpackChangelogGenerator::loadHistory() const
{
    QList<ModpackVersionSnapshot> history;

    QFile file(historyFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return history;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse modpack version history:" << parseError.errorString();
        return history;
    }

    if (!doc.isObject()) {
        return history;
    }

    QJsonObject root = doc.object();
    QJsonArray versionsArray = root["versions"].toArray();
    for (const auto& versionVal : versionsArray) {
        history.append(ModpackVersionSnapshot::fromJson(versionVal.toObject()));
    }

    // Sort by version number
    std::sort(history.begin(), history.end(), [](const ModpackVersionSnapshot& a, const ModpackVersionSnapshot& b) {
        return a.versionNumber < b.versionNumber;
    });

    return history;
}

void ModpackChangelogGenerator::saveHistory(const QList<ModpackVersionSnapshot>& history) const
{
    QJsonArray versionsArray;
    for (const auto& snapshot : history) {
        versionsArray.append(snapshot.toJson());
    }

    QJsonObject root;
    root["format_version"] = 1;
    root["instance_id"] = m_instance->id();
    root["versions"] = versionsArray;

    QJsonDocument doc(root);

    QFile file(historyFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to write modpack version history to" << historyFilePath();
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

ModpackVersionSnapshot ModpackChangelogGenerator::buildSnapshotFromCurrentMods(int versionNumber) const
{
    ModpackVersionSnapshot snapshot;
    snapshot.versionNumber = versionNumber;
    snapshot.versionLabel = QString("v%1").arg(versionNumber);
    snapshot.timestamp = QDateTime::currentDateTime();

    ModFolderModel* modList = m_instance->loaderModList();
    if (!modList) {
        qWarning() << "No mod list available for instance" << m_instance->id();
        return snapshot;
    }

    auto mods = modList->allMods();
    for (auto* mod : mods) {
        if (!mod)
            continue;

        ModSnapshotEntry entry;
        // Use the mod_id for identification; fall back to name if unavailable
        entry.modId = mod->mod_id();
        if (entry.modId.isEmpty()) {
            entry.modId = mod->name();
        }

        entry.name = mod->name();
        if (entry.name.isEmpty()) {
            entry.name = mod->fileinfo().completeBaseName();
        }

        entry.version = mod->version();
        entry.fileName = mod->fileinfo().fileName();

        snapshot.mods.append(entry);
    }

    return snapshot;
}

ModpackVersionSnapshot ModpackChangelogGenerator::createSnapshot()
{
    auto history = loadHistory();

    int nextVersion = 1;
    if (!history.isEmpty()) {
        nextVersion = history.last().versionNumber + 1;
    }

    auto snapshot = buildSnapshotFromCurrentMods(nextVersion);
    history.append(snapshot);
    saveHistory(history);

    qDebug() << "Created modpack snapshot v" << nextVersion << "with" << snapshot.mods.size() << "mods";
    return snapshot;
}

ModpackChangelog ModpackChangelogGenerator::compareSnapshots(const ModpackVersionSnapshot& oldSnapshot,
                                                             const ModpackVersionSnapshot& newSnapshot)
{
    ModpackChangelog changelog;
    changelog.fromVersion = oldSnapshot.versionNumber;
    changelog.toVersion = newSnapshot.versionNumber;

    // Build lookup maps keyed by mod_id
    QMap<QString, ModSnapshotEntry> oldMods;
    for (const auto& mod : oldSnapshot.mods) {
        oldMods[mod.modId] = mod;
    }

    QMap<QString, ModSnapshotEntry> newMods;
    for (const auto& mod : newSnapshot.mods) {
        newMods[mod.modId] = mod;
    }

    // Find added and updated mods
    for (auto it = newMods.constBegin(); it != newMods.constEnd(); ++it) {
        const QString& modId = it.key();
        const ModSnapshotEntry& newMod = it.value();

        if (!oldMods.contains(modId)) {
            // Added mod
            ChangelogEntry entry;
            entry.modName = newMod.name;
            changelog.addedMods.append(entry);
        } else {
            // Check if updated (different version)
            const ModSnapshotEntry& oldMod = oldMods[modId];
            if (!newMod.version.isEmpty() && !oldMod.version.isEmpty() && newMod.version != oldMod.version) {
                ChangelogEntry entry;
                entry.modName = newMod.name;
                entry.oldVersion = oldMod.version;
                entry.newVersion = newMod.version;
                changelog.updatedMods.append(entry);
            } else if (newMod.fileName != oldMod.fileName && (newMod.version.isEmpty() || oldMod.version.isEmpty())) {
                // File changed but version info not available — still counts as updated
                ChangelogEntry entry;
                entry.modName = newMod.name;
                entry.oldVersion = oldMod.version;
                entry.newVersion = newMod.version;
                changelog.updatedMods.append(entry);
            }
        }
    }

    // Find removed mods
    for (auto it = oldMods.constBegin(); it != oldMods.constEnd(); ++it) {
        const QString& modId = it.key();
        const ModSnapshotEntry& oldMod = it.value();

        if (!newMods.contains(modId)) {
            ChangelogEntry entry;
            entry.modName = oldMod.name;
            changelog.removedMods.append(entry);
        }
    }

    return changelog;
}

ModpackChangelog ModpackChangelogGenerator::generateChangelog()
{
    auto history = loadHistory();

    // Get the previous snapshot (if any)
    ModpackVersionSnapshot previousSnapshot;
    if (!history.isEmpty()) {
        previousSnapshot = history.last();
    }

    // Create a snapshot of the current state but DO NOT save it
    int nextVersion = previousSnapshot.versionNumber + 1;
    auto currentSnapshot = buildSnapshotFromCurrentMods(nextVersion);

    // If there was no previous snapshot, return all mods as added
    if (previousSnapshot.versionNumber == 0) {
        ModpackChangelog changelog;
        changelog.fromVersion = 0;
        changelog.toVersion = currentSnapshot.versionNumber;
        for (const auto& mod : currentSnapshot.mods) {
            ChangelogEntry entry;
            entry.modName = mod.name;
            changelog.addedMods.append(entry);
        }
        return changelog;
    }

    return compareSnapshots(previousSnapshot, currentSnapshot);
}

QList<ModpackVersionSnapshot> ModpackChangelogGenerator::getVersionHistory() const
{
    return loadHistory();
}

ModpackVersionSnapshot ModpackChangelogGenerator::getLatestSnapshot() const
{
    auto history = loadHistory();
    if (history.isEmpty()) {
        return {};
    }
    return history.last();
}

int ModpackChangelogGenerator::getSnapshotCount() const
{
    return loadHistory().size();
}

void ModpackChangelogGenerator::clearHistory()
{
    QFile::remove(historyFilePath());
}
