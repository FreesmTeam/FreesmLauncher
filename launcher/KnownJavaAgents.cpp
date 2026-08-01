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

#include "KnownJavaAgents.h"

namespace {
const std::vector<Injectors::KnownJavaAgent>& javaAgents()
{
    static const std::vector javaAgents{
        Injectors::KnownJavaAgent{ .uid = "moe.yushi.authlibinjector", .prefix = "moe.yushi:authlibinjector", .name = "authlib-injector" },
        Injectors::KnownJavaAgent{ .uid = "org.unmojang.loki", .prefix = "org.unmojang:Loki", .name = "Loki" }
    };
    return javaAgents;
}

template <typename Proj>
const Injectors::KnownJavaAgent* findBy(Proj proj, const QString& value)
{
    auto& agents = javaAgents();
    auto it = std::ranges::find(agents, value, proj);
    if (it != agents.end()) {
        return &*it;
    }

    return nullptr;
}
}  // namespace

namespace Injectors {
const std::vector<KnownJavaAgent>& getJavaAgents()
{
    return javaAgents();
}

const KnownJavaAgent* findByUid(const QString& uid)
{
    return findBy(&KnownJavaAgent::uid, uid);
}

const KnownJavaAgent* findByPrefix(const QString& prefix)
{
    return findBy(&KnownJavaAgent::prefix, prefix);
}

const KnownJavaAgent& fallback()
{
    return javaAgents().front();
}
}  // namespace Injectors
