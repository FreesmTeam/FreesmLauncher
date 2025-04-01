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

#include "ElybyAuthStep.h"

#include "Application.h"
#include "ElybyRefreshStep.h"
#include "Logging.h"
#include "net/NetUtils.h"

ElybyAuthStep::ElybyAuthStep(AccountData* data, QString password) : AuthStep(data), m_password(password) {}

void ElybyAuthStep::perform()
{
    QUrl url("https://authserver.ely.by/auth/authenticate");
    QString elybyAuthTemplate = R"XXX(
{
    "username": "%1",
    "password": "%2",
    "clientToken": "%3",
    "requestUser": false
}
)XXX";

    auto elybyAuthData = elybyAuthTemplate.arg(m_data->accountLogin, m_password, APPLICATION->getElybyClientID());
    m_response.reset(new QByteArray());
    m_request = Net::Upload::makeByteArray(url, m_response, elybyAuthData.toUtf8());

    m_task.reset(new NetJob("ElybyAuthStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &ElybyAuthStep::onRequestDone);

    m_task->start();
    qDebug() << "Getting authorization token for Elyby account";
}

void ElybyAuthStep::onRequestDone()
{
    qCDebug(authCredentials()) << *m_response;
    if (m_request->error() != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << m_request->error();
        emit finished(AccountTaskState::STATE_OFFLINE, tr("Failed to get authorization for Elyby: %1").arg(m_request->errorString()));
    }

    auto jsonResponse = QJsonDocument::fromJson(*m_response);

    m_data->yggdrasilToken.token = jsonResponse["accessToken"].toString();

    m_data->clientID = jsonResponse["clientToken"].toString();

    auto profile = jsonResponse["selectedProfile"].toObject();
    m_data->minecraftProfile.id = profile["id"].toString();
    m_data->minecraftProfile.name = profile["name"].toString();
    m_data->minecraftProfile.skin.url = "http://skinsystem.ely.by/skins/" + m_data->minecraftProfile.name + ".png";

    emit finished(AccountTaskState::STATE_WORKING, tr("Got authorization for Elyby"));
}