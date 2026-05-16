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

#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QString>

class ModFolderModel;
class MinecraftInstance;

/**
 * Represents a single mod entry in a version snapshot.
 */
struct ModSnapshotEntry {
    QString modId;       // Unique mod ID (from mod metadata)
    QString name;        // Human-readable name
    QString version;     // Mod version string
    QString fileName;    // File name on disk

    QJsonObject toJson() const;
    static ModSnapshotEntry fromJson(const QJsonObject& obj);
};

/**
 * Represents a version snapshot of a modpack's mod list at a specific point in time.
 */
struct ModpackVersionSnapshot {
    int versionNumber = 0;
    QString versionLabel;           // User-facing label, e.g. "v3"
    QDateTime timestamp;
    QList<ModSnapshotEntry> mods;

    QJsonObject toJson() const;
    static ModpackVersionSnapshot fromJson(const QJsonObject& obj);
};

/**
 * Represents a single change entry in a changelog.
 */
struct ChangelogEntry {
    QString modName;
    QString oldVersion;  // Only for Updated
    QString newVersion;  // Only for Updated
};

/**
 * The result of comparing two version snapshots.
 */
struct ModpackChangelog {
    int fromVersion = 0;
    int toVersion = 0;
    QList<ChangelogEntry> addedMods;
    QList<ChangelogEntry> removedMods;
    QList<ChangelogEntry> updatedMods;

    /** Format the changelog as a clean human-readable string. */
    QString formatAsText() const;

    /** Format the changelog as Markdown. */
    QString formatAsMarkdown() const;

    /** Returns true if there are no changes. */
    bool isEmpty() const;
};

/**
 * Handles creating version snapshots and generating changelogs for modpacks.
 *
 * Version history is stored as a JSON file in the instance's root directory.
 */
class ModpackChangelogGenerator {
   public:
    explicit ModpackChangelogGenerator(MinecraftInstance* instance);

    /**
     * Creates a new version snapshot from the current mod list.
     * Returns the newly created snapshot.
     */
    ModpackVersionSnapshot createSnapshot();

    /**
     * Compares two snapshots and returns a changelog describing the differences.
     */
    static ModpackChangelog compareSnapshots(const ModpackVersionSnapshot& oldSnapshot, const ModpackVersionSnapshot& newSnapshot);

    /**
     * Generates a changelog by creating a new snapshot and comparing with the previous one.
     * This is the main entry point for the "Generate Changelog" button.
     * Returns the generated changelog.
     */
    ModpackChangelog generateChangelog();

    /**
     * Returns all stored version snapshots, ordered by version number.
     */
    QList<ModpackVersionSnapshot> getVersionHistory() const;

    /**
     * Returns the latest snapshot, or a default-constructed one if none exists.
     */
    ModpackVersionSnapshot getLatestSnapshot() const;

    /**
     * Returns the number of stored snapshots.
     */
    int getSnapshotCount() const;

    /**
     * Deletes all version history.
     */
    void clearHistory();

   private:
    /**
     * Builds a snapshot from the current state of the mod folder.
     */
    ModpackVersionSnapshot buildSnapshotFromCurrentMods(int versionNumber) const;

    /**
     * Loads the version history from disk.
     */
    QList<ModpackVersionSnapshot> loadHistory() const;

    /**
     * Saves the version history to disk.
     */
    void saveHistory(const QList<ModpackVersionSnapshot>& history) const;

    /**
     * Returns the path to the version history JSON file.
     */
    QString historyFilePath() const;

    MinecraftInstance* m_instance;
};
