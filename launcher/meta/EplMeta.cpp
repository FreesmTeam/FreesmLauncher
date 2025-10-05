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

#include "Application.h"
#include "BuildConfig.h"
#include "JsonFormat.h"

#include "EplMeta.h"

QUrl Meta::EplMeta::url() const
{
    auto s = APPLICATION->settings();
    QString metaOverride = s->get("EPLMetaURLOverride").toString();
    if (metaOverride.isEmpty()) {
        return QUrl(BuildConfig.EPL_META_URL).resolved(localFilename());
    }
    return QUrl(metaOverride).resolved(localFilename());
}

void Meta::EplMeta::parse(const QJsonObject& obj)
{
    const auto overrides = obj["overrides"].toObject();
    const auto authlibs = overrides["com.mojang:authlib"].toObject();
    const auto extras = obj["extras"].toObject();
    if (authlibs.isEmpty() || extras.isEmpty()) {
        throw ParseException(QObject::tr("Invalid EPL metadata"));
    }

    for (auto it = authlibs.constBegin(); it != authlibs.constEnd(); ++it) {
        m_authlibs[it.key()] = it.value().toObject();
    }
    m_authlibInjector = extras["authlib-injector"].toObject();

    if (m_authlibs.empty() || m_authlibInjector.isEmpty()) {
        throw ParseException(QObject::tr("Invalid EPL metadata"));
    }
}

QJsonObject Meta::EplMeta::overrideFromVersion(const QString& version)
{
    if (const auto it = m_authlibs.find(version); it != m_authlibs.end()) {
        return it->second;
    }
    return {};
}

QJsonObject Meta::EplMeta::authlibInjector()
{
    return m_authlibInjector;
}