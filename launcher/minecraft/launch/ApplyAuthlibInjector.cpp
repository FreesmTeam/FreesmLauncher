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

#include <algorithm>

#include "ApplyAuthlibInjector.h"

ApplyAuthlibInjector::ApplyAuthlibInjector(LaunchTask* parent, RuntimeContext& ctx, Net::Mode netMode) : ApplyLibraryOverride(parent, ctx, netMode) {}

ApplyAuthlibInjector::~ApplyAuthlibInjector() = default;

void ApplyAuthlibInjector::executeTask()
{
    startMetaTask("moe.yushi.authlibinjector");
}

void ApplyAuthlibInjector::onMetaRequestDone(const Meta::VersionList::Ptr& versionList)
{
    auto versions = versionList->versions();
    auto it = std::ranges::find_if(versions, [](const auto& version) { return version && version->isRecommended(); });

    if (it == versions.end()) {
        emit logLine(tr("No recommended authlib-injector version could be found"), MessageLevel::Error);
        emitFailed("No recommended authlib-injector version could be found");
        return;
    }

    const auto& recommendedVersion = *it;

    startApplyTask(recommendedVersion);
}