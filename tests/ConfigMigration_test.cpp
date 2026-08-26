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

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QString>
#include <QTest>
#include <algorithm>

#include "ConfigMigration.h"

namespace {
bool configEqual(const QString& lhsPath, const QString& rhsPath)
{
    QSettings lhs(lhsPath, QSettings::IniFormat), rhs(rhsPath, QSettings::IniFormat);
    auto lhsKeys = lhs.allKeys(), rhsKeys = rhs.allKeys();

    std::ranges::sort(lhsKeys);
    std::ranges::sort(rhsKeys);

    if (lhsKeys != rhsKeys) {
        return false;
    }

    return std::ranges::all_of(lhsKeys, [&lhs, &rhs](const auto& key) { return lhs.value(key) == rhs.value(key); });
}
}  // namespace

class ConfigMigrationTest : public QObject {
    Q_OBJECT

   private slots:
    void test_TrivialCopy()
    {
        class Identity {
           public:
            ConfigMigration::Setting operator()(const ConfigMigration::Setting& s, const QString&) const { return s; }
        };

        auto dataDir = QDir(QFINDTESTDATA("testdata/ConfigMigration"));
        auto from = dataDir.absoluteFilePath("prismlauncher.cfg");
        auto to1 = "prismlauncher.out1.cfg";
        auto to2 = "prismlauncher.out2.cfg";
        auto to3 = "prismlauncher.out3.cfg";

        QFile::remove(to1);
        QFile::remove(to2);
        QFile::remove(to3);

        ConfigMigration::migrate(from, to1);
        ConfigMigration::migrate(from, to2, Identity{});
        ConfigMigration::migrate(from, to3, Identity{}, Identity{});

        QVERIFY(configEqual(from, to1));
        QVERIFY(configEqual(from, to2));
        QVERIFY(configEqual(from, to3));
    }

    void test_Modify()
    {
        auto dataDir = QDir(QFINDTESTDATA("testdata/ConfigMigration"));
        auto from = dataDir.absoluteFilePath("prismlauncher.cfg");
        auto to = "prismlauncher.modified.out.cfg";

        QFile::remove(to);

        ConfigMigration::migrate(
            from, to,
            [](ConfigMigration::Setting setting, QString group) {
                if (group.isEmpty() && setting.key == "CatFit") {
                    setting.value = "skinny";
                }
                if (group.isEmpty() && setting.key == "MaxMemAlloc") {
                    setting.value = "8192";
                }
                return setting;
            },
            [](ConfigMigration::Setting setting, QString group) {
                if (group.startsWith("Cat") && setting.value.canConvert<int>()) {
                    setting.value = setting.value.toInt() * 2;
                }
                return setting;
            });

        QVERIFY(configEqual(to, dataDir.absoluteFilePath("prismlauncher.modified.expected.cfg")));
    }

    void test_Filter()
    {
        auto dataDir = QDir(QFINDTESTDATA("testdata/ConfigMigration"));
        auto from = dataDir.absoluteFilePath("prismlauncher.cfg");
        auto to = "prismlauncher.filtered.out.cfg";

        QFile::remove(to);

        ConfigMigration::migrate(from, to, [](ConfigMigration::Setting setting, QString group) {
            if (group.startsWith("UI") || setting.value.toString().startsWith("/Users/cat/Library/Application Support/PrismLauncher") ||
                setting.key.endsWith("Geometry")) {
                return ConfigMigration::Setting{};
            }
            return setting;
        });

        QVERIFY(configEqual(to, dataDir.absoluteFilePath("prismlauncher.filtered.expected.cfg")));
    }
};

QTEST_GUILESS_MAIN(ConfigMigrationTest)

#include "ConfigMigration_test.moc"
