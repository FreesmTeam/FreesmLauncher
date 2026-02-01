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

#include <QInputDialog>

#include "Application.h"
#include "Logging.h"
#include "net/NetUtils.h"
#include "net/RawHeaderProxy.h"

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

    const auto headerProxy =
        new Net::RawHeaderProxy(QList<Net::HeaderPair>{ { "Content-Type", "application/json" }, { "Accept", "application/json" } });
    m_request->addHeaderProxy(headerProxy);
    // RawHeaderProxy::addHeaderProxy takes ownership of the proxy, so no cleanup is required

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
    "requestUser": false,
    "agent": {
	"name":"Minecraft",
	"version":1
    }
}
)XXX";
    } else {
        return R"XXX(
{
    "accessToken": "%1",
    "clientToken": "%2",
    "requestUser": false,
    "selectedProfile": {
        "id": "%3",
        "name": "%4"
    }
}
)XXX";
    }
}

QString CustomAuthStep::fillRequest()
{
    if (m_action == AuthFlow::Action::Login) {
        return requestTemplate().arg(m_data->accountLogin, m_password, clientID());
    } else {
        return requestTemplate().arg(m_data->yggdrasilToken.token, m_data->clientID, m_data->minecraftProfile.id,
                                     m_data->minecraftProfile.name);
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

    if (!jsonResponse["selectedProfile"].isNull()) {
        auto profile = jsonResponse["selectedProfile"].toObject();
        m_data->minecraftProfile.id = profile["id"].toString();
        m_data->minecraftProfile.name = profile["name"].toString();
    }

    const QJsonArray profiles = jsonResponse["availableProfiles"].toArray();
    if (profiles.size() > 1) {
        const auto profileName = [](const auto& profile) {
            auto obj = profile.toObject();
            return obj["name"].toString();
        };

        QStringList list;
        std::ranges::transform(profiles, std::back_inserter(list), profileName);

        bool ok = false;
        QString selectedProfileName =
            QInputDialog::getItem(nullptr, tr("Select profile"), tr("Select profile for this account"), list, 0, false, &ok);

        if (!ok) {
            return false;
        }

        const auto it = std::ranges::find(profiles, selectedProfileName, profileName);
        if (it != profiles.end()) {
            auto profileObj = it->toObject();
            m_data->minecraftProfile = MinecraftProfile{ .id = profileObj["id"].toString(), .name = profileObj["name"].toString() };
        } else {
            return false;
        }
    }

    if (profiles.size() == 1 && m_data->minecraftProfile.id.isEmpty()) {
        auto profileObj = profiles.first().toObject();
        m_data->minecraftProfile = MinecraftProfile{ .id = profileObj["id"].toString(), .name = profileObj["name"].toString() };
    }

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
