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
#pragma once

#include <QDialog>

namespace Ui {
class ReloadRequiredDialog;
}

class ReloadRequiredDialog : public QDialog {
    Q_OBJECT

   public:
    enum ResultCode {
        Reload = 10,
        DontReload = 11,
    };

    explicit ReloadRequiredDialog(QWidget* parent = 0);
    ~ReloadRequiredDialog() = default;

   private:
    Ui::ReloadRequiredDialog* ui;
};
