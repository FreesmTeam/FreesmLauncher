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

#pragma once

#include <QString>

class MinecraftAccount;

/*!
 * Account identifier that can disambiguate profile ID conflicts when type/auth URL are present.
 */
class AccountIdentifier {
   public:
    AccountIdentifier() = delete;

    /*!
     * Constructs identifier from user provided input.
     * If input is invalid, isValid() considered to be false.
     *
     * @param str "ProfileID[:type[:AuthURL]]" formatted string, where ProfileID is the Minecraft profile ID, type is account type (msa /
     * elyby / custom / offline) and AuthURL is authlib-injector auth URL, used for matching only custom accounts.
     * Only first two ':' characters are separators.
     */
    explicit AccountIdentifier(const QString& str);

    /*!
     * Constructs identifier from profile ID, account type and authlib-injector auth URL.
     *
     * @param profileId Minecraft profile ID
     * @param accountType Account type ("msa" / "elyby" / "custom" / "offline" / "")
     * @param authUrl authlib-injector auth URL, used for matching only custom accounts
     */
    explicit AccountIdentifier(QString profileId, QString accountType, QString authUrl = {});

    /*!
     * Constructs identifier from existing Minecraft account.
     *
     * @param account a valid Minecraft account
     */
    explicit AccountIdentifier(const MinecraftAccount& account);

    /*!
     * @return whether the identifier is valid
     */
    bool isValid() const;

    /*!
     * @return profile ID
     */
    QString profileId() const;

    /*!
     * @return account type string
     */
    QString accountType() const;

    /*!
     * @return authlib-injector auth URL
     */
    QString authUrl() const;

    /*!
     * Matches specified account with identifier. Returns false if the identifier is not valid.
     * Empty account type matches any account with the same profile ID.
     * For custom accounts, a non-empty auth URL must also match.
     *
     * @param account a valid Minecraft account
     * @return whether the account match this identifier
     */
    bool matches(const MinecraftAccount& account) const;

   private:
    QString m_profileId;
    QString m_accountType;
    QString m_authUrl;
};

enum class AccountFindError { NoError, InvalidId, NotFound, Ambiguous };

struct AccountFindResult {
    int index = -1;
    AccountFindError error = AccountFindError::NotFound;

    bool found() const { return index >= 0 && error == AccountFindError::NoError; }
};