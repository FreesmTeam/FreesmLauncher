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

#include "CustomAuthStep.h"

#include "Application.h"
#include "Logging.h"
#include "net/NetUtils.h"

#include <utility>

CustomAuthStep::CustomAuthStep(AccountData* data, QString password) : AuthStep(data), m_password(std::move(password)) {}

void CustomAuthStep::perform()
{
    const QString requestUrl("/auth/authenticate");
    const QUrl url(m_data->authUrl + requestUrl);

    const QString requestTemplate = R"XXX(
{
    "username": "%1",
    "password": "%2",
    "clientToken": "%3",
    "requestUser": false
}
)XXX";
    const QString requestData = requestTemplate.arg(m_data->accountLogin, m_password, QString::number(0));
    m_response.reset(new QByteArray());
    m_request = Net::Upload::makeByteArray(url, m_response, requestData.toUtf8());

    m_task.reset(new NetJob(authType() + "AuthStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &CustomAuthStep::onRequestDone);

    m_task->start();
    qDebug() << "Getting authorization token for " + authType() + " account";
}

bool CustomAuthStep::parseResponse()
{
    qCDebug(authCredentials()) << *m_response;
    if (m_request->error() != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << m_request->error();
        return false;
    }

    auto jsonResponse = QJsonDocument::fromJson(*m_response);

    m_data->yggdrasilToken.token = jsonResponse["accessToken"].toString();

    m_data->clientID = jsonResponse["clientToken"].toString();

    auto profile = jsonResponse["selectedProfile"].toObject();
    m_data->minecraftProfile.id = profile["id"].toString();
    m_data->minecraftProfile.name = profile["name"].toString();

    return true;
}

void CustomAuthStep::onRequestDone()
{
    if (!parseResponse()) {
        emit finished(AccountTaskState::STATE_OFFLINE,
                      tr("Failed to get authorization for %1 account: %2").arg(authType(), m_request->errorString()));
        return;
    }
    emit finished(AccountTaskState::STATE_WORKING, tr("Got authorization for %1 account").arg(authType()));
}