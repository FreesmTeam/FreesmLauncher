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

#include <QString>
#include <optional>

#include "ApplyLibraryOverride.h"

class ApplyAuthPatch : public ApplyLibraryOverride {
    Q_OBJECT
   public:
    enum class Stage { Ely, Injector };

    explicit ApplyAuthPatch(LaunchTask* parent, RuntimeContext& ctx, Net::Mode netMode, Stage stage);
    ~ApplyAuthPatch() override;

   protected:
    void executeTask() override;

   protected slots:
    void onMetaRequestDone(const Meta::VersionList::Ptr& versionList) override;
    void apply(const Meta::Version::Ptr& version) override;

   private:
    void executeEly();
    void executeInjector();

    Stage m_stage;
    QString m_decidedInjectorUid;
    std::optional<QString> m_decidedInjectorVersion;
};
