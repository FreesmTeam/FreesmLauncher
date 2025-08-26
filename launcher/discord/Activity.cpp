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

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "Application.h"
#include "BaseInstance.h"
#include "RunningInstance.h"

#include "Activity.h"

namespace {
enum class DiscordActivityType { Playing = 0, Streaming = 1, Listening = 2, Watching = 3, Custom = 4, Competing = 5 };

QJsonValue makeNonce()
{
    return QUuid::createUuid().toString();
}

QJsonValue makeActivityDetails(const InstancePtr& instance)
{
    return "Minecraft " + instance->getVersionString();
}

QJsonObject makeActivityTimestamps(const RunningInstance& instance)
{
    QJsonObject timestamps;

    timestamps["start"] = instance.startedAt.toSecsSinceEpoch();

    return timestamps;
}

[[maybe_unused]] QJsonObject makeActivityAssets(const InstancePtr& instance)
{
    QJsonObject assets;

    assets["large_image"] = instance->iconKey();

    return assets;
}

QJsonObject makeActivity(const RunningInstance& instance)
{
    QJsonObject activity;

    activity["name"] = "Playing Minecraft";
    activity["type"] = static_cast<int>(DiscordActivityType::Playing);
    activity["details"] = makeActivityDetails(instance.ptr());
    activity["timestamps"] = makeActivityTimestamps(instance);
    // activity["assets"] = makeActivityAssets(instance);

    return activity;
}
}  // namespace

QByteArray generateActivity(const RunningInstance& instance)
{
    QJsonObject request;

    QJsonObject args;
    args["pid"] = APPLICATION->applicationPid();
    args["activity"] = makeActivity(instance);

    request["args"] = args;
    request["nonce"] = makeNonce();
    request["cmd"] = "SET_ACTIVITY";

    return QJsonDocument(request).toJson();
}

QByteArray generateEmptyActivity()
{
    QJsonObject request;

    QJsonObject args;
    args["pid"] = APPLICATION->applicationPid();
    args["activity"] = QJsonValue::Null;

    request["args"] = args;
    request["nonce"] = makeNonce();
    request["cmd"] = "SET_ACTIVITY";

    return QJsonDocument(request).toJson();
}