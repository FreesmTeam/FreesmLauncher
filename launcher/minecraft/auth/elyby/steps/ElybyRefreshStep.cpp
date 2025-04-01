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

#include "ElybyRefreshStep.h"

#include "ElybyAuthStep.h"

#include "Application.h"
#include "Logging.h"
#include "net/NetUtils.h"

ElybyRefreshStep::ElybyRefreshStep(AccountData* data) : AuthStep(data) {}

void ElybyRefreshStep::perform()
{
    QUrl url("https://authserver.ely.by/auth/refresh");
    QString elybyRefreshTemplate = R"XXX(
{
    "accessToken": "%1",
    "clientToken": "%2",
    "requestUser": false
}
)XXX";

    auto elybyRefreshData = elybyRefreshTemplate.arg(m_data->yggdrasilToken.token, m_data->clientID);
    m_response.reset(new QByteArray());
    m_request = Net::Upload::makeByteArray(url, m_response, elybyRefreshData.toUtf8());

    m_task.reset(new NetJob("ElybyRefreshStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &ElybyRefreshStep::onRequestDone);

    m_task->start();
    qDebug() << "Refreshing authorization token for Elyby account";
}

void ElybyRefreshStep::onRequestDone()
{
    qCDebug(authCredentials()) << *m_response;
    if (m_request->error() != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << m_request->error();
        emit finished(AccountTaskState::STATE_OFFLINE, tr("Failed to refresh authorization for Elyby: %1").arg(m_request->errorString()));
    }

    auto jsonResponse = QJsonDocument::fromJson(*m_response);

    m_data->yggdrasilToken.token = jsonResponse["accessToken"].toString();

    auto profile = jsonResponse["selectedProfile"].toObject();
    m_data->minecraftProfile.id = profile["id"].toString();
    m_data->minecraftProfile.name = profile["name"].toString();
    m_data->minecraftProfile.skin.url = "http://skinsystem.ely.by/skins/" + m_data->minecraftProfile.name + ".png";

    emit finished(AccountTaskState::STATE_WORKING, tr("Got authorization for Elyby"));
}