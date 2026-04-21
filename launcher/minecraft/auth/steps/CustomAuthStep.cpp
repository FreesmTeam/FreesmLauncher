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

#include "CustomAuthStep.h"

#include <QDateTime>
#include <QInputDialog>
#include <QJsonDocument>
#include <utility>

#include "Application.h"
#include "Logging.h"
#include "net/NetUtils.h"
#include "net/RawHeaderProxy.h"

CustomAuthStep::CustomAuthStep(AccountData* data, QString password) : AuthStep(data), m_password(std::move(password)) {}

CustomAuthStep::~CustomAuthStep() = default;

void CustomAuthStep::perform()
{
    if (m_data == nullptr) {
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Account data is a null pointer"));
        return;
    }

    const QUrl url(m_data->authUrl + m_data->loginUrl);
    const QJsonDocument body(fillRequest());

    auto [request, response] = Net::Upload::makeByteArray(url, body.toJson());
    m_request = std::move(request);

    m_request->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(
        QList<Net::HeaderPair>{ { "Content-Type", "application/json; charset=utf-8" }, { "Accept", "application/json" } }));

    m_task.reset(new NetJob("CustomAuthStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, [this, response] { onRequestDone(response); });

    m_task->start();
    qDebug() << "Getting authorization token for custom account";
}

QJsonObject CustomAuthStep::fillRequest() const
{
    QJsonObject root;
    root.insert("username", m_data->accountLogin);
    root.insert("password", m_password);

    QJsonObject agent;
    agent.insert("name", "Minecraft");
    agent.insert("version", 1);

    root.insert("agent", agent);

    return root;
}

void CustomAuthStep::onRequestDone(const QByteArray* response)
{
    qCDebug(authCredentials()) << *response;

    if (m_request->error() != QNetworkReply::NoError && m_request->error() != QNetworkReply::ContentAccessDenied) {
        emit finished(AccountTaskState::STATE_OFFLINE, m_request->errorString());
        return;
    }

    QJsonParseError err;
    auto jsonResponse = QJsonDocument::fromJson(*response, &err);

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
    if (!selectedProfile.isEmpty()) {
        m_data->minecraftProfile.id = selectedProfile["id"].toString();
        m_data->minecraftProfile.name = selectedProfile["name"].toString();

        emit finished(AccountTaskState::STATE_WORKING, tr("Got authorization for custom account"));
        return;
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
            emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Profile selection cancelled"));
            return;
        }

        const auto it = std::ranges::find(profiles, selectedProfileName, profileName);
        if (it != profiles.end()) {
            auto profileObj = it->toObject();
            m_data->minecraftProfile = MinecraftProfile{ .id = profileObj["id"].toString(), .name = profileObj["name"].toString() };
        } else {
            // assuming that this will never happen
            emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Something went wrong"));
            return;
        }
    } else if (profiles.size() == 1) {
        auto profileObj = profiles.first().toObject();
        m_data->minecraftProfile = MinecraftProfile{ .id = profileObj["id"].toString(), .name = profileObj["name"].toString() };
    }

    m_data->profileSelectedExplicitly = true;

    emit finished(AccountTaskState::STATE_WORKING, tr("Got authorization for custom account"));
}
