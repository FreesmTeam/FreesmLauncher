// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Freesm Launcher - Minecraft Launcher
 *  Copyright (C) 2025 so5iso4ka <so5iso4ka@icloud.com>
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

#include <unordered_map>

#include "BaseEntity.h"

namespace Meta {

class EplMeta : public BaseEntity {
   public:
    QString localFilename() const override { return "epl_metadata.json"; }
    QUrl url() const override;

    void parse(const QJsonObject& obj) override;

    QJsonObject overrideFromVersion(const QString& version);
    QJsonObject authlibInjector();

   private:
    std::unordered_map<QString, QJsonObject> m_authlibs;
    QJsonObject m_authlibInjector;
};
}  // namespace Meta