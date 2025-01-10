// SPDX-FileCopyrightText: 2025 Kaeeraa <ilhainshakov@yandex.ru>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 Kaeeraa <ilhainshakov@yandex.ru>
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
 */

#include "ReloadRequiredDialog.h"
#include <QPushButton>
#include "Application.h"
#include "BuildConfig.h"
#include "Markdown.h"
#include "StringUtils.h"
#include "ui_ReloadRequiredDialog.h"

ReloadRequiredDialog::ReloadRequiredDialog(QWidget* parent) : QDialog(parent), ui(new Ui::ReloadRequiredDialog)
{
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

    ui->setupUi(this);

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, [this]() {
        setResult(ResultCode::Reload);
        done(ResultCode::Reload);
    });

    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, [this]() {
        setResult(ResultCode::DontReload);
        done(ResultCode::DontReload);
    });
}

