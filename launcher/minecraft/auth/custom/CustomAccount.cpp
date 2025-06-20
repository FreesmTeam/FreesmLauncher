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

#include "CustomAccount.h"

#include <utility>

CustomAccount::CustomAccount(QObject* parent) : BaseAccount(parent) {}

CustomAccount::CustomAccount(const QString& authUrl, QObject* parent) : CustomAccount(parent)
{
    data.authUrl = authUrl;
}

CustomAccountPtr CustomAccount::createCustom(const QString& login, const QString& authUrl)
{
    CustomAccountPtr account(new CustomAccount(authUrl));
    account->data.type = AccountType::Custom;
    account->data.accountLogin = login;
    return account;
}

shared_qobject_ptr<AuthFlow> CustomAccount::login(QString password)
{
    Q_ASSERT(m_currentTask.get() == nullptr);

    m_currentTask.reset(new AuthFlow(&data, AuthFlow::Action::Login, std::move(password)));
    connect(m_currentTask.get(), &Task::succeeded, this, &CustomAccount::authSucceeded);
    connect(m_currentTask.get(), &Task::failed, this, &CustomAccount::authFailed);
    connect(m_currentTask.get(), &Task::aborted, this, [this] { authFailed(tr("Aborted")); });
    emit activityChanged(true);

    return m_currentTask;
}