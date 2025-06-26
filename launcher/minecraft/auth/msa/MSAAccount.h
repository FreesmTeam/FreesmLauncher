// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QPixmap>
#include <QString>

#include "QObjectPtr.h"
#include "Usable.h"
#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/AuthFlow.h"
#include "minecraft/auth/AuthSession.h"
#include "minecraft/auth/BaseAccount.h"

class Task;
class MSAAccount;

using MSAAccountPtr = shared_qobject_ptr<MSAAccount>;
Q_DECLARE_METATYPE(MSAAccountPtr)

class MSAAccount : public BaseAccount {
    Q_OBJECT
   public: /* construction */
    //! Default constructor
    explicit MSAAccount(QObject* parent = 0);

   public: /* static methods */
    static MSAAccountPtr createBlankMSA();

   public: /* methods */
    shared_qobject_ptr<AuthFlow> login(bool useDeviceCode = false);
};
