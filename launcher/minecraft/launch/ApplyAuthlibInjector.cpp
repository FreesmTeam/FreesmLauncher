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

ApplyAuthlibInjector::ApplyAuthlibInjector(LaunchTask* parent, AuthSessionPtr session)
    : LaunchStep(parent), m_session(session), m_instance(m_parent->instance())
{}

void ApplyAuthlibInjector::executeTask()
{
    const QString downloadUrl = "https://github.com/yushijinhun/authlib-injector/releases/download/v1.2.5/authlib-injector-1.2.5.jar";
    m_request = Net::Download::makeFile(downloadUrl, "authlib-injector.jar");

    m_task.reset(new NetJob("Download authlib-injector", APPLICATION->network()));
    m_task->addNetAction(m_request);

    connect(m_task.get(), &NetJob::succeeded, this, &ApplyAuthlibInjector::onRequestDone);
    connect(m_task.get(), &NetJob::failed, this, &ApplyAuthlibInjector::emitFailed);
    connect(m_task.get(), &NetJob::aborted, this, [this] { emitFailed(tr("Aborted")); });

    m_task->start();
}

void ApplyAuthlibInjector::onRequestDone()
{
    m_session->authlib_injector_ready = true;
    emitSucceeded();
}