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

#include "ConfigMigration.h"

namespace ConfigMigration {
Setting ExcludeByPrefix::operator()(const Setting& s, const QString&) const
{
    if (s.value.toString().startsWith(prefix)) {
        return {};
    }
    return s;
}

Setting TransformPineconeMCSettings::operator()(const Setting& setting, const QString& group) const
{
    auto elySkinSettingProj = [](int elyPatchPreference) -> int {
        switch (elyPatchPreference) {
            case 0: {  // Always
                return 1;
            }
            case 1: {  // When using Ely and Offline accounts
                return 3;
            }
            case 2: {  // When using Ely accounts
                return 2;
            }
            case 3: {  // Never
                return 0;
            }
            default: {
                return 2;
            }
        }
    };

    if (group.isEmpty() && setting.key == "ElyPatchPreference" && !setting.value.isNull()) {
        return Setting{ "UseElySkins", elySkinSettingProj(setting.value.toInt()) };
    }
    return setting;
}
}  // namespace ConfigMigration
