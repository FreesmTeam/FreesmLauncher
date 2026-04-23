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

#include "meta/Version.h"
#include "meta/VersionList.h"
#include "net/Mode.h"

class MinecraftInstance;
struct RuntimeContext;

class ApplyLibraryOverride : public LaunchStep {
    Q_OBJECT
   public:
    explicit ApplyLibraryOverride(LaunchTask* parent, RuntimeContext& ctx, Net::Mode netMode);
    ~ApplyLibraryOverride() override;

    bool canAbort() const override { return false; }

   protected:
    void startMetaTask(const QString& uid);
    void startApplyTask(const Meta::Version::Ptr& version);

   protected slots:
    virtual void onMetaRequestDone(const Meta::VersionList::Ptr& versionList) = 0;
    virtual void apply(const Meta::Version::Ptr& version);

   protected:
    MinecraftInstance* m_instance;
    RuntimeContext* m_ctx;
    Task::Ptr m_task;
    Net::Mode m_netMode;
};
