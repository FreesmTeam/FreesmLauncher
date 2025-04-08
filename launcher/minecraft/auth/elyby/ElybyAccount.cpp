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

#include "ElybyAccount.h"

ElybyAccount::ElybyAccount(QObject* parent) {}

ElybyAccountPtr ElybyAccount::createElyby(const QString& login)
{
    ElybyAccountPtr account(new ElybyAccount());
    account->data.type = AccountType::Elyby;
    account->data.accountLogin = login;
    return account;
}

shared_qobject_ptr<AuthFlow> ElybyAccount::login(QString password)
{
    Q_ASSERT(m_currentTask.get() == nullptr);

    m_currentTask.reset(new AuthFlow(&data, AuthFlow::Action::Login, password));
    connect(m_currentTask.get(), &Task::succeeded, this, &ElybyAccount::authSucceeded);
    connect(m_currentTask.get(), &Task::failed, this, &ElybyAccount::authFailed);
    connect(m_currentTask.get(), &Task::aborted, this, [this] { authFailed(tr("Aborted")); });
    emit activityChanged(true);

    return m_currentTask;
}

shared_qobject_ptr<AuthFlow> ElybyAccount::refresh()
{
    if (m_currentTask) {
        return m_currentTask;
    }

    m_currentTask.reset(new AuthFlow(&data, AuthFlow::Action::Refresh));

    connect(m_currentTask.get(), &Task::succeeded, this, &ElybyAccount::authSucceeded);
    connect(m_currentTask.get(), &Task::failed, this, &ElybyAccount::authFailed);
    connect(m_currentTask.get(), &Task::aborted, this, [this] { authFailed(tr("Aborted")); });
    emit activityChanged(true);
    return m_currentTask;
}

void ElybyAccount::authFailed(QString reason)
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
            data.yggdrasilToken = Token();
            data.clientID = QString();
            data.accountLogin = QString();

            data.validity_ = Validity::None;

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