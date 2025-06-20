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

#include "minecraft/auth/custom/steps/CustomRefreshStep.h"

class ElybyRefreshStep : public CustomRefreshStep {
    Q_OBJECT

   public:
    explicit ElybyRefreshStep(AccountData* data);
    virtual ~ElybyRefreshStep() = default;

    QString describe() override { return tr("Ely.by account refreshing"); }

   protected:
    QString authType() override { return "Ely.by"; }

    QString authUrl() override { return "https://authserver.ely.by"; }

    void setSkin();

   protected slots:
    void onRequestDone() override;
};
