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

// Huge thanks to SophrTemin
#include "GruvboxTheme.h"

#include <QObject>

QString GruvboxTheme::id()
{
    return "gruvbox";
}

QString GruvboxTheme::name()
{
    return QObject::tr("Gruvbox");
}

QPalette GruvboxTheme::colorScheme()
{
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(50, 48, 47));
    darkPalette.setColor(QPalette::WindowText, QColor(249, 245, 215));
    darkPalette.setColor(QPalette::Base, QColor(80, 73, 69));
    darkPalette.setColor(QPalette::AlternateBase, QColor(50, 48, 47));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(255, 0, 0));
    darkPalette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Text, QColor(251, 241, 199));
    darkPalette.setColor(QPalette::Button, QColor(60, 56, 54));
    darkPalette.setColor(QPalette::ButtonText, QColor(249, 245, 215));
    darkPalette.setColor(QPalette::BrightText, QColor(204, 164, 29));
    darkPalette.setColor(QPalette::Link, QColor(131, 165, 152));
    darkPalette.setColor(QPalette::Highlight, QColor(131, 165, 152));
    darkPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::PlaceholderText, Qt::darkGray);
    return fadeInactive(darkPalette, fadeAmount(), fadeColor());
}

double GruvboxTheme::fadeAmount()
{
    return 0.5;
}

QColor GruvboxTheme::fadeColor()
{
    return QColor(60, 56, 54);
}

bool GruvboxTheme::hasStyleSheet()
{
    return true;
}

QString GruvboxTheme::appStyleSheet()
{
    return "QToolTip { color: #fbf1c7; background-color: #504945; border: 1px solid white; }";
}
QString GruvboxTheme::tooltip()
{
    return "";
}