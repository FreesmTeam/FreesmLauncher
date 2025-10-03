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

#include "ApplyAuthlibInjector.h"

#include "Application.h"
#include "launch/LaunchTask.h"
#include "meta/EplMeta.h"

ApplyAuthlibInjector::ApplyAuthlibInjector(LaunchTask* parent, const AuthSessionPtr& session) : LaunchStep(parent), m_session(session) {}

void ApplyAuthlibInjector::executeTask()
{
    auto meta = APPLICATION->eplMetadata();
    if (meta->isLoaded())
        return onRequestDone();

    m_metaTask = meta->loadTask();
    connect(m_metaTask.get(), &Task::succeeded, this, &ApplyAuthlibInjector::onRequestDone);
    connect(m_metaTask.get(), &Task::failed, this, [this] {
        emit logLine(tr("Couldn't fetch EPL metadata from %1").arg(APPLICATION->eplMetadata()->url().toString()), MessageLevel::Error);
        emitFailed(tr("Couldn't fetch EPL metadata"));
    });
    connect(m_metaTask.get(), &Task::aborted, this, [this] { emitFailed(tr("Aborted")); });

    if (!m_metaTask->isRunning()) {
        m_metaTask->start();
    }
}

void ApplyAuthlibInjector::downloadFile()
{
    const QString downloadUrl = APPLICATION->eplMetadata()->authlibInjector()["url"].toString();
    auto request = Net::Download::makeFile(downloadUrl, "authlib-injector.jar");

    m_netTask.reset(new NetJob("Download authlib-injector", APPLICATION->network()));
    m_netTask->addNetAction(request);

    connect(m_netTask.get(), &NetJob::succeeded, this, &ApplyAuthlibInjector::onRequestDone);
    connect(m_netTask.get(), &NetJob::failed, this, [this](QString reason) {
        emit logLine(tr("Couldn't load authlib-injector: %1").arg(std::move(reason)), MessageLevel::Error);
        emitFailed(tr("Download failed"));
    });
    connect(m_netTask.get(), &NetJob::aborted, this, [this] { emitFailed(tr("Aborted")); });

    m_netTask->start();
}

void ApplyAuthlibInjector::onRequestDone()
{
    if (!checkFile()) {
        if (m_netTask != nullptr) {
            emit logLine(tr("The checksum of the downloaded file does not match."), MessageLevel::Error);
            emitFailed(tr("Checksum mismatch"));
            return;
        }

        return downloadFile();
    }

    m_session->authlib_injector_ready = true;

    emitSucceeded();
}

bool ApplyAuthlibInjector::checkFile()
{
    const auto authlibInjector = APPLICATION->eplMetadata()->authlibInjector();

    QFile file("authlib-injector.jar");
    file.open(QIODevice::ReadOnly);
    if (!file.exists() || !file.isOpen() ||
        QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex() != authlibInjector["sha256"].toString()) {
        return false;
    }

    return true;
}