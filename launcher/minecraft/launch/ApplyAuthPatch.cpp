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

#include "ApplyAuthPatch.h"

#include "KnownJavaAgents.h"
#include "launch/LaunchTask.h"
#include "minecraft/PackProfile.h"

ApplyAuthPatch::ApplyAuthPatch(LaunchTask* parent, RuntimeContext& ctx, Net::Mode netMode, Stage stage)
    : ApplyLibraryOverride(parent, ctx, netMode), m_stage(stage)
{}

ApplyAuthPatch::~ApplyAuthPatch() = default;

void ApplyAuthPatch::executeTask()
{
    emit logLine("", MessageLevel::Launcher);
    switch (m_stage) {
        case Stage::Ely: {
            executeEly();
            break;
        }
        case Stage::Injector: {
            executeInjector();
            break;
        }
    }
}

void ApplyAuthPatch::onMetaRequestDone(const Meta::VersionList::Ptr& versionList)
{
    switch (m_stage) {
        case Stage::Ely: {
            auto libraries = m_instance->getPackProfile()->getProfile()->getLibraries();

            auto authlibIt =
                std::ranges::find_if(libraries, [](const LibraryPtr& p) { return p && p->artifactPrefix() == "com.mojang:authlib"; });
            if (authlibIt == libraries.end()) {
                emit logLine(tr("Could not apply ely patch: authlib not found; falling back to injector"), MessageLevel::Warning);
                m_stage = Stage::Injector;
                return executeInjector();
            }

            auto authlibVersion = (*authlibIt)->version();

            auto patchedVersions = versionList->versions();

            auto patchedAuthlibIt = std::ranges::find_if(
                patchedVersions, [authlibVersion](const Meta::Version::Ptr& p) { return p->version() == authlibVersion; });
            if (patchedAuthlibIt == patchedVersions.end()) {
                emit logLine(tr("Could not apply ely patch: no suitable patched authlib found; falling back to injector"),
                             MessageLevel::Warning);
                m_stage = Stage::Injector;
                return executeInjector();
            }

            startApplyTask(*patchedAuthlibIt);
            break;
        }
        case Stage::Injector: {
            auto versions = versionList->versions();
            Meta::Version::Ptr recommended;
            Meta::Version::Ptr preferred;

            std::ranges::for_each(versions, [this, &recommended, &preferred](const auto& version) {
                if (!version) {
                    return;
                }
                if (!recommended && version->isRecommended()) {
                    recommended = version;
                }
                if (m_decidedInjectorVersion.has_value() && version->descriptor() == *m_decidedInjectorVersion) {
                    preferred = version;
                }
            });

            auto decided = Injectors::findByUid(m_decidedInjectorUid);

            if (!recommended && !preferred) {
                if (m_decidedInjectorUid == Injectors::fallback().uid) {
                    emit logLine(tr("No suitable injector version could be found"), MessageLevel::Error);
                    emitFailed("No suitable injector version could be found");
                    return;
                }

                emit logLine(tr("No suitable %1 version could be found; falling back to %2")
                                 .arg(decided ? decided->name : m_decidedInjectorUid, Injectors::fallback().name),
                             MessageLevel::Warning);
                m_decidedInjectorUid = Injectors::fallback().uid;
                m_decidedInjectorVersion.reset();

                return startMetaTask(m_decidedInjectorUid);
            }

            if (!preferred && m_decidedInjectorVersion.has_value()) {
                emit logLine(tr("No %1 %2 found; falling back to %3")
                                 .arg(decided ? decided->name : m_decidedInjectorUid, *m_decidedInjectorVersion, recommended->descriptor()),
                             MessageLevel::Warning);
            }

            startApplyTask(preferred != nullptr ? preferred : recommended);
            break;
        }
    }
}
void ApplyAuthPatch::apply(const Meta::Version::Ptr& version)
{
    switch (m_stage) {
        case Stage::Ely: {
            QList<LibraryPtr>& libraries = m_instance->getPackProfile()->getProfile()->libraries();
            auto it = std::ranges::find_if(libraries, [](const LibraryPtr& p) { return p && p->artifactPrefix() == "com.mojang:authlib"; });
            if (it != libraries.end()) {
                libraries.erase(it);
            }
            emit logLine(tr("Using ely patch version %1").arg(version->descriptor()), MessageLevel::Launcher);
            ApplyLibraryOverride::apply(version);
            break;
        }
        case Stage::Injector: {
            emit logLine(tr("Using %1 %2").arg(version->name(), version->descriptor()), MessageLevel::Launcher);
            ApplyLibraryOverride::apply(version);
            break;
        }
    }
}

void ApplyAuthPatch::executeEly()
{
    startMetaTask("by.ely.authlib");
}

void ApplyAuthPatch::executeInjector()
{
    auto preferred = m_parent->instance()->settings()->get("InjectorUid").toString();
    auto preferredVersion = m_parent->instance()->settings()->get("InjectorVersion").toString();

    if (Injectors::findByUid(preferred) == nullptr) {
        m_decidedInjectorUid = Injectors::fallback().uid;
    } else {
        m_decidedInjectorUid = preferred;
        if (!preferredVersion.isEmpty()) {
            m_decidedInjectorVersion = preferredVersion;
        }
    }

    startMetaTask(m_decidedInjectorUid);
}
