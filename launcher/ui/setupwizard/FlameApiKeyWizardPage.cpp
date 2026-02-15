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

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "Application.h"
#include "ui/GuiUtil.h"

#include "FlameApiKeyWizardPage.h"

FlameAPIKeyWizardPage::FlameAPIKeyWizardPage(QWidget* parent) : BaseWizardPage(parent)
{
    auto layout = new QVBoxLayout{ this };
    m_titleLabel = new QLabel{ this };
    m_descriptionLabel = new QLabel{ this };
    m_descriptionLabel->setWordWrap(true);
    m_fetchButton = new QPushButton{ this };

    connect(m_fetchButton, &QPushButton::clicked, this, [this] {
        const auto& apiKey = GuiUtil::fetchFlameKey(this);
        if (!apiKey.isEmpty()) {
            APPLICATION->settings()->set("FlameKeyOverride", apiKey);
            APPLICATION->updateCapabilities();
        }
    });

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_descriptionLabel);
    layout->addWidget(m_fetchButton);

    setLayout(layout);

    FlameAPIKeyWizardPage::retranslate();
}

void FlameAPIKeyWizardPage::initializePage()
{
    APPLICATION->settings()->set("FlameKeyShouldBeFetchedOnStartup", false);
}

void FlameAPIKeyWizardPage::retranslate()
{
    m_titleLabel->setText(
        tr(R"(<html><head/><body><p><span style="font-size:14pt; font-weight:600;">Fetch CurseForge API key</span></p></body></html>)"));
    m_descriptionLabel->setText(
        tr("Using the official CurseForge app's API key may break CurseForge's terms of service but should allow Freesm Launcher to "
           "download all mods in a modpack without you needing to download any of them manually. This can be done later in the settings."));
    m_fetchButton->setText(tr("Fetch Official Launcher's Key"));
}
