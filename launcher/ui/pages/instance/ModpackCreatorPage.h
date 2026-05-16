// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Freesm Launcher - Minecraft Launcher
 *  Copyright (C) 2026 FreesmModders
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

#include <QWidget>

#include "minecraft/MinecraftInstance.h"
#include "ui/pages/BasePage.h"

namespace Ui {
class ModpackCreatorPage;
}

class ModpackCreatorPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit ModpackCreatorPage(MinecraftInstance* inst, QWidget* parent = nullptr);
    virtual ~ModpackCreatorPage();

    virtual QString displayName() const override { return tr("Modpack Creator"); }
    virtual QIcon icon() const override
    {
        auto icon = QIcon::fromTheme("project-development");
        if (icon.isNull())
            icon = QIcon::fromTheme("package-x-generic");
        if (icon.isNull())
            icon = QIcon::fromTheme("preferences-plugin");
        return icon;
    }
    virtual QString id() const override { return "modpack-creator"; }
    virtual bool apply() override;
    virtual QString helpPage() const override { return "Modpack-Creator"; }
    virtual bool shouldDisplay() const override;
    void retranslate() override;

   private slots:
    void on_markAsModpackBtn_clicked();
    void on_generateChangelogBtn_clicked();
    void on_copyChangelogBtn_clicked();
    void on_clearHistoryBtn_clicked();
    void on_modpackNameEdit_textChanged(const QString& text);
    void on_modpackAuthorEdit_textChanged(const QString& text);

   private:
    void updateUI();
    void refreshVersionInfo();

    Ui::ModpackCreatorPage* ui;
    MinecraftInstance* m_inst;
};
