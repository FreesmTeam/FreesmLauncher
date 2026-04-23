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

#include "minecraft/launch/ApplyLibraryOverride.h"

class ApplyElyPatch : public ApplyLibraryOverride {
    Q_OBJECT
   public:
    explicit ApplyElyPatch(LaunchTask* parent, RuntimeContext& ctx, Net::Mode netMode);
    ~ApplyElyPatch() override;

   protected:
    void executeTask() override;

   protected slots:
    void onMetaRequestDone(const Meta::VersionList::Ptr& versionList) override;
    void apply(const Meta::Version::Ptr& version) override;

private:
    void applyAuthlibInjector();
    bool m_fallbackToAuthlibInjector = false;
};
