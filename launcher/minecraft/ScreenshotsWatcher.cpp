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
#include <QFileSystemWatcher>
#include <QImage>
#include <QThread>

#include "Application.h"
#include "MinecraftInstance.h"

#include "ScreenshotsWatcher.h"

ScreenshotsWatcher::ScreenshotsWatcher(const QString& path) : QObject(), m_path(path)
{
    auto watcher = new QFileSystemWatcher({ path });

    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &ScreenshotsWatcher::dirUpdated);

    m_watcher = std::make_unique<QFileSystemWatcher>(watcher);
}

void ScreenshotsWatcher::dirUpdated()
{
    QDir d(m_path, "*.png", QDir::Time, QDir::Files);
    if (!d.exists() || !d.count()) {
        qDebug() << "Invalid screenshots dir";
        return;
    }
    QString path = m_path + '/' + d[0];

    auto clipboard = Application::clipboard();
    if (clipboard == nullptr) {
        qDebug() << "Clipboard ptr is null, aborting copying";
        return;
    }

    QImage img;
    bool loaded = img.load(path);
    for (int i = 0; i < 5 && !loaded; i++) {
        QThread::msleep(200);
        loaded = img.load(path);
    }
    if (!loaded) {
        qDebug() << "Image wasn't loaded in 1 sec, aborting copying";
        return;
    }

    clipboard->setImage(img);
}
