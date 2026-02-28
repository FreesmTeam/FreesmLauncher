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

#include <QClipboard>
#include <QImage>

#include "Application.h"
#include "FileSystem.h"
#include "MinecraftInstance.h"

#include "ScreenshotsWatcher.h"

ScreenshotsWatcher::ScreenshotsWatcher(const QString& path) : m_watcher(QStringList{ path })
{
    m_retryTimer.setSingleShot(true);
    m_retryTimer.setInterval(200);
    connect(&m_retryTimer, &QTimer::timeout, this, &ScreenshotsWatcher::tryCopy);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &ScreenshotsWatcher::dirUpdated);
}

void ScreenshotsWatcher::dirUpdated(const QString& path)
{
    QDir d(path, "*.png", QDir::Time, QDir::Files);
    if (!d.exists() || !d.count()) {
        qDebug() << "Invalid screenshots dir";
        return;
    }

    QString file = FS::PathCombine(path, d[0]);
    if (file == m_pendingPath)
        return;

    m_pendingPath = file;
    m_attempt = 0;
    m_lastSize = QFileInfo(m_pendingPath).size();

    scheduleRetry();
}

void ScreenshotsWatcher::tryCopy()
{
    const QFileInfo fileInfo(m_pendingPath);
    if (!fileInfo.exists())
        return;

    if (m_lastSize == 0 || m_lastSize != fileInfo.size()) {
        m_lastSize = fileInfo.size();
        scheduleRetry();
        return;
    }
    m_lastSize = fileInfo.size();

    QImage img;
    bool loaded = img.load(m_pendingPath);

    if (!loaded) {
        scheduleRetry();
        return;
    }

    const auto clipboard = QApplication::clipboard();
    if (clipboard == nullptr) {
        qDebug() << "Clipboard is null";
        return;
    }

    clipboard->setImage(img);
}

void ScreenshotsWatcher::scheduleRetry()
{
    ++m_attempt;
    if (m_attempt > 5) {
        qDebug() << "Image wasn't loaded after 5 retries";
        return;
    }

    m_retryTimer.start();
}
