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

class ElybyAccount;

using ElybyAccountPtr = shared_qobject_ptr<ElybyAccount>;
Q_DECLARE_METATYPE(ElybyAccountPtr)

class ElybyAccount : public BaseAccount {
    Q_OBJECT
   public: /*constructions*/
    explicit ElybyAccount(QObject* parent = 0);

   public: /* static methods */
    static ElybyAccountPtr createElyby(const QString& login);

   public: /* methods */
    shared_qobject_ptr<AuthFlow> login(QString password);

    shared_qobject_ptr<AuthFlow> refresh() override;

   private slots:
    void authFailed(QString reason) override;
};