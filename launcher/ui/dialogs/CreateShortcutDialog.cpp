// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (C) 2025 Yihe Li <winmikedows@hotmail.com>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include <QLayout>
#include <QPushButton>

#include "Application.h"
#include "BuildConfig.h"
#include "CreateShortcutDialog.h"
#include "ui_CreateShortcutDialog.h"

#include "ui/dialogs/IconPickerDialog.h"

#include "BaseInstance.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "icons/IconList.h"
#include "minecraft/ShortcutUtils.h"

CreateShortcutDialog::CreateShortcutDialog(InstancePtr instance, QWidget* parent)
    : QDialog(parent), ui(new Ui::CreateShortcutDialog), m_instance(instance)
{
    ui->setupUi(this);

    InstIconKey = instance->iconKey();
    ui->iconButton->setIcon(APPLICATION->icons()->getIcon(InstIconKey));
    ui->instNameTextBox->setText(instance->name());

    m_QuickJoinSupported = instance->traits().contains("feature:is_quick_play_singleplayer");
    if (!m_QuickJoinSupported) {
        ui->worldTarget->hide();
        ui->worldSelectionBox->hide();
    }

    // Populate save targets
    if (!DesktopServices::isFlatpak()) {
        QString desktopDir = FS::getDesktopDir();
        QString applicationDir = FS::getApplicationsDir();

        if (!desktopDir.isEmpty())
            ui->saveTargetSelectionBox->addItem("Desktop", QVariant::fromValue(SaveTarget::Desktop));

        if (!applicationDir.isEmpty())
            ui->saveTargetSelectionBox->addItem("Applications", QVariant::fromValue(SaveTarget::Applications));
    }
    ui->saveTargetSelectionBox->addItem("Other...", QVariant::fromValue(SaveTarget::Other));
}

CreateShortcutDialog::~CreateShortcutDialog()
{
    delete ui;
}

void CreateShortcutDialog::on_iconButton_clicked()
{
    IconPickerDialog dlg(this);
    dlg.execWithSelection(InstIconKey);

    if (dlg.result() == QDialog::Accepted) {
        InstIconKey = dlg.selectedIconKey;
        ui->iconButton->setIcon(APPLICATION->icons()->getIcon(InstIconKey));
    }
}

void CreateShortcutDialog::on_overrideAccountCheckbox_stateChanged(int state)
{
    ui->accountOptionsGroup->setEnabled(state == Qt::Checked);
}

void CreateShortcutDialog::on_accountSelectionBox_currentIndexChanged(int index) {}

void CreateShortcutDialog::on_targetCheckbox_stateChanged(int state)
{
    ui->targetOptionsGroup->setEnabled(state == Qt::Checked);
}

void CreateShortcutDialog::on_worldSelectionBox_currentIndexChanged(int index) {}

void CreateShortcutDialog::on_serverAddressBox_textChanged(const QString& arg1) {}

void CreateShortcutDialog::targetChanged() {}

// Real work
void CreateShortcutDialog::createShortcut()
{
    QString targetString = tr("instance");
    QStringList extraArgs;
    if (ui->targetCheckbox->isChecked()) {
        if (ui->worldTarget->isChecked()) {
            targetString = tr("world");
            extraArgs = { "--world", /* world ID */ };
        } else if (ui->serverTarget->isChecked()) {
            targetString = tr("server");
            extraArgs = { "--server", /* server address */ };
        }
    }

    auto target = ui->saveTargetSelectionBox->currentData().value<SaveTarget>();
    auto name = ui->instNameTextBox->text();
    if (target == SaveTarget::Desktop)
        ShortcutUtils::createInstanceShortcutOnDesktop({ m_instance.get(), name, targetString, this, extraArgs, InstIconKey });
    else if (target == SaveTarget::Applications)
        ShortcutUtils::createInstanceShortcutInApplications({ m_instance.get(), name, targetString, this, extraArgs, InstIconKey });
    else
        ShortcutUtils::createInstanceShortcutInOther({ m_instance.get(), m_instance->name(), targetString, this, extraArgs, InstIconKey });
}
