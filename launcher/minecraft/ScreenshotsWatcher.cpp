// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Freesm Launcher - Minecraft Launcher
 *  Copyright (C) 2026 so5iso4ka <so5iso4ka@icloud.com>
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
#include <QtConcurrent>
#include <algorithm>

#include "Application.h"
#include "FileSystem.h"
#include "MinecraftInstance.h"
#include "PNGUtils.h"

#include "ScreenshotsWatcher.h"

ScreenshotsWatcher::ScreenshotsWatcher(const QString& path) : m_watcher(QStringList{ path })
{
    m_retryTimer.setSingleShot(true);
    connect(&m_retryTimer, &QTimer::timeout, this, &ScreenshotsWatcher::tryCopy);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &ScreenshotsWatcher::dirUpdated);
    connect(&m_futureWatcher, &QFutureWatcher<LoadResult>::finished, this, &ScreenshotsWatcher::onTryFinished);
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

    scheduleRetry();
}

void ScreenshotsWatcher::tryCopy()
{
    m_futureWatcher.setFuture(QtConcurrent::run(QThreadPool::globalInstance(), tryLoadPNG, m_pendingPath));
}

void ScreenshotsWatcher::onTryFinished()
{
    auto future = m_futureWatcher.future();
    if (!future.isValid()) {
        return;
    }

    auto [status, img, path] = future.result();

    if (path != m_pendingPath) {
        return;
    }

    switch (status) {
        case LoadResult::Status::Ok: {
            break;
        }
        case LoadResult::Status::Retry: {
            scheduleRetry();
            return;
        }
        case LoadResult::Status::Failed: {
            qDebug() << "Unable to load the image:" << path;
            return;
        }
    }

    auto clipboard = QApplication::clipboard();
    if (!clipboard) {
        qDebug() << "Clipboard is null";
        return;
    }

    clipboard->setImage(img);
}

void ScreenshotsWatcher::scheduleRetry()
{
    ++m_attempt;
    if (m_attempt > 12) {  // 100 + 200 + 400 + 800 + 1000 * 8 = 9500ms
        qDebug() << "Image wasn't loaded in time: " << m_pendingPath;
        return;
    }

    m_retryTimer.start(std::min(100 << (m_attempt - 1), 1000));
}

ScreenshotsWatcher::LoadResult ScreenshotsWatcher::tryLoadPNG(QString path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return { LoadResult::Status::Retry, {}, path };
    }

    if (file.size() > qint64{ 512 * 1024 * 1024 }) {
        return { LoadResult::Status::Failed, {}, path };
    }

    auto data = file.readAll();
    if (!PNGUtils::isCompletePNG(data)) {
        return { LoadResult::Status::Retry, {}, path };
    }

    QImage img;
    if (!img.loadFromData(data, "PNG")) {
        return { LoadResult::Status::Failed, {}, path };
    }

    return { LoadResult::Status::Ok, img, path };
}
