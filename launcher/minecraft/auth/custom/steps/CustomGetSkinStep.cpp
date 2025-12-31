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

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QUrl>

#include "Application.h"
#include "QObjectPtr.h"
#include "net/Download.h"
#include "net/RawHeaderProxy.h"

#include "CustomGetSkinStep.h"

namespace {
QString getSkinUrlFromProperties(const QString& encodedProps)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromBase64(encodedProps.toUtf8()), &err);
    if (err.error != QJsonParseError::NoError) {
        return {};
    }

    auto json = doc.object();
    auto url = json["textures"].toObject()["SKIN"].toObject()["url"].toString();

    return { url };
}
}  // namespace

CustomGetSkinStep::CustomGetSkinStep(AccountData* data) : GetSkinStep(data) {}

CustomGetSkinStep::~CustomGetSkinStep() = default;

void CustomGetSkinStep::perform()
{
    const QUrl url(m_data->authUrl + QString("/sessionserver/session/minecraft/profile/%1").arg(m_data->minecraftProfile.id));

    m_response = std::make_shared<QByteArray>();
    m_request = Net::Download::makeByteArray(url, m_response);

    const auto headerProxy =
        new Net::RawHeaderProxy(QList<Net::HeaderPair>{ { "Content-Type", "application/json" }, { "Accept", "application/json" } });
    m_request->addHeaderProxy(headerProxy);
    // RawHeaderProxy::addHeaderProxy takes ownership of the proxy, so no cleanup is required

    m_task = makeShared<NetJob>("CustomGetSkinStep", APPLICATION->network());
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &CustomGetSkinStep::onPropertiesFetchDone);

    m_task->start();
}

QString CustomGetSkinStep::describe()
{
    return tr("Getting custom account skin");
}

void CustomGetSkinStep::onPropertiesFetchDone()
{
    if (m_request->error() != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << m_request->error();
        emit finished(AccountTaskState::STATE_WORKING, tr("Failed to get skin for account: %1").arg(m_request->errorString()));
        return;
    }

    QJsonParseError err;
    auto jsonResponse = QJsonDocument::fromJson(*m_response, &err);
    if (err.error != QJsonParseError::NoError) {
        emit finished(AccountTaskState::STATE_WORKING, tr("Failed to get skin for account: %1").arg(err.error));
        return;
    }

    auto props = jsonResponse["properties"].toArray();

    for (auto prop : props) {
        auto obj = prop.toObject();
        if (obj["name"].toString() == "textures") {
            QString url = getSkinUrlFromProperties(obj["value"].toString());
            if (url.isEmpty()) {
                break;
            }
            m_data->minecraftProfile.skin.url = url;
            return GetSkinStep::perform();
        }
    }

    emit finished(AccountTaskState::STATE_WORKING, tr("Failed to get skin for account: %1").arg(err.error));
}