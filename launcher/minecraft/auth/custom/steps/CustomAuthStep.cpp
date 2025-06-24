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

CustomAuthStep::CustomAuthStep(AccountData* data, AuthFlow::Action action, QString password)
    : AuthStep(data), m_password(std::move(password)), m_action(action)
{}

void CustomAuthStep::perform()
{
    const QUrl url(authUrl() + requestUrl());
    const QString requestData = fillRequest();

    m_response.reset(new QByteArray());
    m_request = Net::Upload::makeByteArray(url, m_response, requestData.toUtf8());

    m_task.reset(new NetJob(authType() + "AuthStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &CustomAuthStep::onRequestDone);

    m_task->start();
    qDebug() << "Getting authorization token for " + authType() + " account";
}

QString CustomAuthStep::requestUrl()
{
    return m_action == AuthFlow::Action::Login ? m_data->loginUrl : m_data->refreshUrl;
}

QString CustomAuthStep::requestTemplate()
{
    if (m_action == AuthFlow::Action::Login) {
        return R"XXX(
{
    "username": "%1",
    "password": "%2",
    "clientToken": "%3",
    "requestUser": false
}
)XXX";
    } else {
        return R"XXX(
{
    "accessToken": "%1",
    "clientToken": "%2",
    "requestUser": false
}
)XXX";
    }
}

QString CustomAuthStep::fillRequest()
{
    if (m_action == AuthFlow::Action::Login) {
        return requestTemplate().arg(m_data->accountLogin, m_password, clientID());
    } else {
        return requestTemplate().arg(m_data->yggdrasilToken.token, m_data->clientID);
    }
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