// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Freesm Launcher - Minecraft Launcher
 *  Copyright (C) 2026 so5iso4ka <so5iso4ka@icloud.com>
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

#include "CustomRefreshStep.h"

#include <QJsonDocument>

#include "Application.h"
#include "Logging.h"
#include "net/RawHeaderProxy.h"

CustomRefreshStep::CustomRefreshStep(AccountData* data) : AuthStep(data) {}

CustomRefreshStep::~CustomRefreshStep() = default;

void CustomRefreshStep::perform()
{
    if (m_data == nullptr) {
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Account data is a null pointer"));
        return;
    }

    const QUrl url(m_data->authUrl + m_data->refreshUrl);
    const QJsonDocument request(fillRequest());

    m_response = std::make_shared<QByteArray>();
    m_request = Net::Upload::makeByteArray(url, m_response, request.toJson());

    // RawHeaderProxy::addHeaderProxy takes ownership of the proxy, so no cleanup is required
    m_request->addHeaderProxy(new Net::RawHeaderProxy(
        QList<Net::HeaderPair>{ { "Content-Type", "application/json; charset=utf-8" }, { "Accept", "application/json" } }));

    m_task.reset(new NetJob("CustomRefreshStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &CustomRefreshStep::onRequestDone);

    m_task->start();
    qDebug() << "Getting authorization token for custom account";
}

QString CustomRefreshStep::describe()
{
    return tr("Refreshing custom account");
}

QJsonObject CustomRefreshStep::fillRequest() const
{
    QJsonObject root;
    root.insert("accessToken", m_data->yggdrasilToken.token);
    root.insert("clientToken", m_data->clientID);

    if (m_data->profileSelectedExplicitly) {
        QJsonObject selectedProfile;
        selectedProfile.insert("id", m_data->minecraftProfile.id);
        selectedProfile.insert("name", m_data->minecraftProfile.name);

        root.insert("selectedProfile", selectedProfile);
    }

    return root;
}

void CustomRefreshStep::onRequestDone()
{
    qCDebug(authCredentials()) << *m_response;

    if (m_request->error() != QNetworkReply::NoError && m_request->error() != QNetworkReply::ContentAccessDenied) {
        emit finished(AccountTaskState::STATE_OFFLINE, m_request->errorString());
        return;
    }

    QJsonParseError err;
    auto jsonResponse = QJsonDocument::fromJson(*m_response, &err);

    if (err.error != QJsonParseError::NoError) {
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Error while parsing JSON response: %1").arg(err.errorString()));
        return;
    }

    if (m_request->error() == QNetworkReply::ContentAccessDenied) {
        const QString msg = jsonResponse["errorMessage"].toString() == "Invalid credentials. Invalid username or password."
                                ? tr("Invalid credentials. Invalid username or password.")
                                : m_request->errorString();

        emit finished(AccountTaskState::STATE_FAILED_HARD, msg);
        return;
    }

    m_data->yggdrasilToken.token = jsonResponse["accessToken"].toString();
    m_data->yggdrasilToken.validity = Validity::Certain;
    m_data->yggdrasilToken.issueInstant = QDateTime::currentDateTimeUtc();

    m_data->clientID = jsonResponse["clientToken"].toString();

    QJsonObject selectedProfile = jsonResponse["selectedProfile"].toObject();
    if (selectedProfile.isEmpty()) {
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("No profile selected"));
        return;
    }

    m_data->minecraftProfile.id = selectedProfile["id"].toString();
    m_data->minecraftProfile.name = selectedProfile["name"].toString();

    m_data->profileSelectedExplicitly = false;

    emit finished(AccountTaskState::STATE_WORKING, tr("Refreshed custom account"));
}
