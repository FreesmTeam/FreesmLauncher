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

#pragma once

#include "minecraft/auth/BaseAccount.h"

class CustomAccount;

using CustomAccountPtr = shared_qobject_ptr<CustomAccount>;
Q_DECLARE_METATYPE(CustomAccountPtr)

class CustomAccount : public BaseAccount {
    Q_OBJECT
   public: /*constructions*/
    explicit CustomAccount(QObject* parent = 0);

   public: /* static methods */
    static CustomAccountPtr createCustom(const QString& login, const QString& authUrl, const QString& loginUrl, const QString& refreshUrl);

   public: /* methods */
    shared_qobject_ptr<AuthFlow> login(QString password);
};