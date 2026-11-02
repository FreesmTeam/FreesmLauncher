// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 *
 */

#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <utility>

#include "BuildConfig.h"
#include "net/HeaderProxy.h"

namespace Modrinth {

struct DownloadMeta {
    QString reason;
    QString gameVersion;
    QString loader;

    bool isEmpty() const { return reason.isEmpty(); }

    QByteArray toJson() const
    {
        QJsonObject obj;
        if (!reason.isEmpty()) {
            obj["reason"] = reason;
        }
        if (!gameVersion.isEmpty()) {
            obj["game_version"] = gameVersion;
        }
        if (!loader.isEmpty()) {
            obj["loader"] = loader;
        }
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
};

class DownloadHeaderProxy final : public Net::HeaderProxy {
   public:
    explicit DownloadHeaderProxy(DownloadMeta meta) : m_meta(std::move(meta)) {}

    QList<Net::HeaderPair> headers(const QNetworkRequest& request) const override
    {
        if (request.url().host() == BuildConfig.MODRINTH_DOWNLOAD_HOST && !m_meta.isEmpty()) {
            return { { .headerName = "modrinth-download-meta", .headerValue = m_meta.toJson() } };
        }
        return {};
    }

   private:
    DownloadMeta m_meta;
};

}  // namespace Modrinth
