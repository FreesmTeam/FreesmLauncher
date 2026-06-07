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

#include <QSet>
#include <utility>

#include "MinecraftAccount.h"
#include "UrlUtils.h"

#include "AccountIdentifier.h"

AccountIdentifier::AccountIdentifier(const QString& str)
{
    const auto firstSep = str.indexOf(':');
    if (firstSep == -1) {
        m_profileId = str;
        return;
    }

    m_profileId = str.left(firstSep);

    const auto secondSep = str.indexOf(':', firstSep + 1);
    if (secondSep == -1) {
        m_accountType = str.mid(firstSep + 1);
        return;
    }

    m_accountType = str.mid(firstSep + 1, secondSep - firstSep - 1);
    m_authUrl = str.mid(secondSep + 1);
}

AccountIdentifier::AccountIdentifier(QString profileId, QString accountType, QString authUrl)
    : m_profileId(std::move(profileId)), m_accountType(std::move(accountType)), m_authUrl(std::move(authUrl))
{}

AccountIdentifier::AccountIdentifier(const MinecraftAccount& account)
    : m_profileId(account.profileId()), m_accountType(account.typeString()), m_authUrl(account.accountData()->authUrl)
{}

bool AccountIdentifier::isValid() const
{
    if (m_profileId.size() != 32)
        return false;

    static const QSet<QString> validAccountTypes = { "", "msa", "elyby", "custom", "offline" };
    if (!validAccountTypes.contains(m_accountType))
        return false;

    if (m_accountType == "custom" && !m_authUrl.isEmpty()) {
        QString errorString;
        UrlUtils::httpFromUserInput(m_authUrl, &errorString);
        if (!errorString.isEmpty())
            return false;
    }

    return true;
}

QString AccountIdentifier::profileId() const
{
    return m_profileId;
}

QString AccountIdentifier::accountType() const
{
    return m_accountType;
}

QString AccountIdentifier::authUrl() const
{
    if (m_authUrl.isEmpty())
        return {};
    return UrlUtils::httpFromUserInput(m_authUrl, nullptr).toString(QUrl::StripTrailingSlash);
}

bool AccountIdentifier::matches(const MinecraftAccount& account) const
{
    if (!isValid())
        return false;

    if (account.profileId() != m_profileId)
        return false;

    if (!m_accountType.isEmpty() && m_accountType != account.typeString())
        return false;

    if (m_accountType == "custom" && !m_authUrl.isEmpty()) {
        if (authUrl() != account.accountData()->authUrl)
            return false;
    }

    return true;
}