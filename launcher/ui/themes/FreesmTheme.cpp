// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
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
 *      Copyright 2024-2024 Freesm Launcher Contributors
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
#include "FreesmTheme.h"

#include <QObject>

QString FreesmTheme::id()
{
    return "freesm";
}

QString FreesmTheme::name()
{
    return QObject::tr("Freesm");
}

bool FreesmTheme::hasColorScheme()
{
    return true;
}

QPalette FreesmTheme::colorScheme()
{
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(30, 30, 46));
    darkPalette.setColor(QPalette::WindowText, QColor(186, 194, 222));
    darkPalette.setColor(QPalette::Base, QColor(24, 24, 37));
    darkPalette.setColor(QPalette::AlternateBase, QColor(30, 30, 46));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(222, 229, 252));
    darkPalette.setColor(QPalette::ToolTipText, QColor(222, 229, 252));
    darkPalette.setColor(QPalette::Text, QColor(205, 214, 244));
    darkPalette.setColor(QPalette::Button, QColor(49, 50, 68));
    darkPalette.setColor(QPalette::ButtonText, QColor(205, 214, 244));
    darkPalette.setColor(QPalette::BrightText, QColor(186, 194, 222));
    darkPalette.setColor(QPalette::Link, QColor(180, 190, 254));
    darkPalette.setColor(QPalette::Highlight, QColor(180, 190, 254));
    darkPalette.setColor(QPalette::HighlightedText, QColor(30, 30, 46));
    darkPalette.setColor(QPalette::PlaceholderText, Qt::darkGray);
    return fadeInactive(darkPalette, fadeAmount(), fadeColor());
}

double FreesmTheme::fadeAmount()
{
    return 0.5;
}

QColor FreesmTheme::fadeColor()
{
    return QColor(108, 112, 134);
}

bool FreesmTheme::hasStyleSheet()
{
    return true;
}

QString FreesmTheme::appStyleSheet()
{
    return "QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }";
}
QString FreesmTheme::tooltip()
{
    return "";
}
