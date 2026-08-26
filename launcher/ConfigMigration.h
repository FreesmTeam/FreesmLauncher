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

#include <QFile>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <algorithm>
#include <concepts>
#include <functional>
#include <list>

namespace ConfigMigration {
struct Setting {
    QString key;
    QVariant value;
};

template <typename F>
concept SettingProj =
    std::regular_invocable<F, Setting, QString> && std::is_convertible_v<std::invoke_result_t<F, Setting, QString>, Setting>;

class ExcludeByPrefix {
   public:
    Setting operator()(const Setting& s, const QString&) const;

    QString prefix;
};

class TransformPineconeMCSettings {
   public:
    Setting operator()(const Setting& setting, const QString& group) const;
};

template <SettingProj... Proj>
bool migrate(const QString& oldConfig, const QString& newConfig, Proj... proj)
{
    if (!QFile::exists(oldConfig)) {
        return false;
    }

    QSettings oldSettings(oldConfig, QSettings::IniFormat);
    QSettings newSettings(newConfig, QSettings::IniFormat);

    std::list<QString> groups{ "" };
    {
        auto topLevelGroups = oldSettings.childGroups();
        groups.insert(groups.end(), topLevelGroups.begin(), topLevelGroups.end());
    }

    for (auto it = groups.begin(); it != groups.end(); ++it) {
        oldSettings.beginGroup(*it);
        newSettings.beginGroup(*it);

        if (it != groups.begin()) {
            auto innerGroups = oldSettings.childGroups();
            std::ranges::for_each(innerGroups, [currentGroup = *it](QString& group) { group.prepend(currentGroup + '/'); });
            groups.insert(groups.end(), innerGroups.begin(), innerGroups.end());
        }

        auto keys = oldSettings.childKeys();
        for (const auto& key : keys) {
            Setting setting{ key, oldSettings.value(key) };
            ((setting = std::invoke(proj, setting, *it)), ...);
            if (!setting.key.isEmpty()) {
                newSettings.setValue(setting.key, setting.value);
            }
        }

        newSettings.endGroup();
        oldSettings.endGroup();
    }
    return true;
}
}  // namespace ConfigMigration
