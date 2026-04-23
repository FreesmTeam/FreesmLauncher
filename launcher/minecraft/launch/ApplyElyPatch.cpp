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

#include "minecraft/PackProfile.h"
#include "minecraft/MinecraftInstance.h"

#include "ApplyElyPatch.h"

ApplyElyPatch::ApplyElyPatch(LaunchTask* parent, RuntimeContext& ctx, Net::Mode netMode) : ApplyLibraryOverride(parent, ctx, netMode) {}

ApplyElyPatch::~ApplyElyPatch() = default;

void ApplyElyPatch::executeTask()
{
    startMetaTask("by.ely.authlib");
}

void ApplyElyPatch::onMetaRequestDone(const Meta::VersionList::Ptr& versionList)
{
    if (!m_fallbackToAuthlibInjector) {
        auto libraries = m_instance->getPackProfile()->getProfile()->getLibraries();

        auto authlibIt =
            std::ranges::find_if(libraries, [](const LibraryPtr& p) { return p && p->artifactPrefix() == "com.mojang:authlib"; });
        if (authlibIt == libraries.end()) {
            return applyAuthlibInjector();
        }

        auto authlibVersion = (*authlibIt)->version();

        auto patchedVersions = versionList->versions();

        auto patchedAuthlibIt =
            std::ranges::find_if(patchedVersions, [authlibVersion](const Meta::Version::Ptr& p) { return p->version() == authlibVersion; });
        if (patchedAuthlibIt == patchedVersions.end()) {
            return applyAuthlibInjector();
        }

        startApplyTask(*patchedAuthlibIt);
    } else {
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
}

void ApplyElyPatch::apply(const Meta::Version::Ptr& version)
{
    if (!m_fallbackToAuthlibInjector) {
        QList<LibraryPtr>& libraries = m_instance->getPackProfile()->getProfile()->libraries();
        auto it = std::ranges::find_if(libraries, [](const LibraryPtr& p) { return p && p->artifactPrefix() == "com.mojang:authlib"; });
        if (it != libraries.end()) {
            libraries.erase(it);
        }
    }

    ApplyLibraryOverride::apply(version);
}

void ApplyElyPatch::applyAuthlibInjector()
{
    m_fallbackToAuthlibInjector = true;
    startMetaTask("moe.yushi.authlibinjector");
}