// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Freesm Launcher - Minecraft Launcher
 *  Copyright (C) 2025 so5iso4ka <so5iso4ka@icloud.com>
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

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>
#include <cstddef>

class ScreenshotsWatcher : public QObject {
    Q_OBJECT
   public:
    explicit ScreenshotsWatcher(const QString& path);

   private slots:
    void dirUpdated(const QString& path);
    void tryCopy();

   private:
    void scheduleRetry();

    QFileSystemWatcher m_watcher;
    QString m_pendingPath;
    std::size_t m_attempt{};
    std::size_t m_lastSize{};
    QTimer m_retryTimer;
};
