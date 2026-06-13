// AppearanceWidget.h
// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (C) 2022 Tayou <git@tayou.org>
 *  Copyright (C) 2026 fractal <fractal@nebula-nook.ru>
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

#include <QDialog>

#include <qslider.h>
#include <qspinbox.h>
#include <translations/TranslationsModel.h>
#include <QTextCursor>
#include "ui/widgets/RangeSlider.h"

class QTextCharFormat;
class SettingsObject;

namespace Ui {
class AppearanceWidget;
}

class AppearanceWidget : public QWidget {
    Q_OBJECT

   public:
    explicit AppearanceWidget(bool simple, QWidget* parent = 0);
    virtual ~AppearanceWidget();

   public:
    void applySettings();
    void loadSettings();
    void retranslateUi();

   private:
    void applyIconTheme(int index);
    void applyWidgetTheme(int index);
    void applyCatTheme(int index);
    void applySnowflakePack(int index);
    static void applySnow(bool visible);
    static void applySnowColor(const QString& color);
    void loadThemeSettings();

    static void loadRangeSetting(const QString& prefix, QSpinBox* minBox, QSpinBox* maxBox, RangeSlider* slider);
    static void loadSingleSetting(const QString& key, QSpinBox* spinBox, QSlider* slider);

    void updateConsolePreview();
    void updateCatPreview();

    Ui::AppearanceWidget* m_ui;
    QTextCharFormat m_defaultFormat;
    bool m_themesOnly;
};
