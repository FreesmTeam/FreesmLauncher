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

#include "OfflineAccount.h"

OfflineAccountPtr OfflineAccount::createOffline(const QString& username)
{
    auto account = makeShared<OfflineAccount>();
    account->data.type = AccountType::Offline;
    account->data.yggdrasilToken.token = "0";
    account->data.yggdrasilToken.validity = Validity::Certain;
    account->data.yggdrasilToken.issueInstant = QDateTime::currentDateTimeUtc();
    account->data.yggdrasilToken.extra["userName"] = username;
    account->data.yggdrasilToken.extra["clientToken"] = QUuid::createUuid().toString().remove(QRegularExpression("[{}-]"));
    account->data.minecraftProfile.id = uuidFromUsername(username).toString().remove(QRegularExpression("[{}-]"));
    account->data.minecraftProfile.name = username;
    account->data.minecraftProfile.validity = Validity::Certain;
    return account;
}

shared_qobject_ptr<AuthFlow> OfflineAccount::login()
{
    Q_ASSERT(m_currentTask.get() == nullptr);

    m_currentTask.reset(new AuthFlow(&data, AuthFlow::Action::Login));
    connect(m_currentTask.get(), &Task::succeeded, this, &OfflineAccount::authSucceeded);
    connect(m_currentTask.get(), &Task::failed, this, &OfflineAccount::authFailed);
    connect(m_currentTask.get(), &Task::aborted, this, [this] { authFailed(tr("Aborted")); });
    emit activityChanged(true);
    return m_currentTask;
}