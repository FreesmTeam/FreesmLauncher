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

#include <launch/LaunchStep.h>
#include <minecraft/auth/MinecraftAccount.h>
#include "minecraft/MinecraftInstance.h"
#include "net/Download.h"
#include "net/NetJob.h"

class ApplyAuthlibInjector : public LaunchStep {
    Q_OBJECT
   public:
    explicit ApplyAuthlibInjector(LaunchTask* parent, AuthSessionPtr session);
    ~ApplyAuthlibInjector() override = default;

    bool canAbort() const override { return false; }

   protected:
    void executeTask() override;

   private slots:
    void onRequestDone();

   private:
    AuthSessionPtr m_session;
    MinecraftInstance* m_instance;
    Net::Download::Ptr m_request;
    NetJob::Ptr m_task;
};
