// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Authors: Orochimarufan <orochimarufan.x3@gmail.com>
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "MSAAccount.h"

#include <QColor>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>
#include <QUuid>

#include <QDebug>

#include <QPainter>

#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/AuthFlow.h"

MSAAccount::MSAAccount(QObject* parent) : BaseAccount(parent) {}

MSAAccountPtr MSAAccount::createBlankMSA()
{
    MSAAccountPtr account(new MSAAccount());
    account->data.type = AccountType::MSA;
    return account;
}

shared_qobject_ptr<AuthFlow> MSAAccount::login(bool useDeviceCode)
{
    Q_ASSERT(m_currentTask.get() == nullptr);

    m_currentTask.reset(new AuthFlow(&data, useDeviceCode ? AuthFlow::Action::DeviceCode : AuthFlow::Action::Login));
    connect(m_currentTask.get(), &Task::succeeded, this, &MSAAccount::authSucceeded);
    connect(m_currentTask.get(), &Task::failed, this, &MSAAccount::authFailed);
    connect(m_currentTask.get(), &Task::aborted, this, [this] { authFailed(tr("Aborted")); });
    emit activityChanged(true);
    return m_currentTask;
}

shared_qobject_ptr<AuthFlow> MSAAccount::refresh()
{
    if (m_currentTask) {
        return m_currentTask;
    }

    m_currentTask.reset(new AuthFlow(&data, AuthFlow::Action::Refresh));

    connect(m_currentTask.get(), &Task::succeeded, this, &MSAAccount::authSucceeded);
    connect(m_currentTask.get(), &Task::failed, this, &MSAAccount::authFailed);
    connect(m_currentTask.get(), &Task::aborted, this, [this] { authFailed(tr("Aborted")); });
    emit activityChanged(true);
    return m_currentTask;
}

void MSAAccount::authFailed(QString reason)
{
    switch (m_currentTask->taskState()) {
        case AccountTaskState::STATE_OFFLINE:
        case AccountTaskState::STATE_DISABLED: {
            // NOTE: user will need to fix this themselves.
        }
        case AccountTaskState::STATE_FAILED_SOFT: {
            // NOTE: this doesn't do much. There was an error of some sort.
        } break;
        case AccountTaskState::STATE_FAILED_HARD: {
            if (accountType() == AccountType::MSA) {
                data.msaToken.token = QString();
                data.msaToken.refresh_token = QString();
                data.msaToken.validity = Validity::None;
                data.validity_ = Validity::None;
            } else {
                data.yggdrasilToken.token = QString();
                data.yggdrasilToken.validity = Validity::None;
                data.validity_ = Validity::None;
            }
            emit changed();
        } break;
        case AccountTaskState::STATE_FAILED_GONE: {
            data.validity_ = Validity::None;
            emit changed();
        } break;
        case AccountTaskState::STATE_CREATED:
        case AccountTaskState::STATE_WORKING:
        case AccountTaskState::STATE_SUCCEEDED: {
            // Not reachable here, as they are not failures.
        }
    }
    m_currentTask.reset();
    emit activityChanged(false);
}