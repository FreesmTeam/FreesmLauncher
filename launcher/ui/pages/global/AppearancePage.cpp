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

#include "AppearancePage.h"

#include <QScrollArea>

AppearancePage::AppearancePage(QWidget* parent) : QWidget(parent), m_widget(new AppearanceWidget(false, this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(m_widget);

    layout->addWidget(scrollArea);

    if (const auto widgetLayout = qobject_cast<QVBoxLayout*>(m_widget->layout())) {
        widgetLayout->addStretch();
    }
}

bool AppearancePage::apply()
{
    m_widget->applySettings();
    return true;
}

void AppearancePage::retranslate()
{
    m_widget->retranslateUi();
}
