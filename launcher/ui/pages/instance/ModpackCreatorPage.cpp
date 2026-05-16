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

#include "ModpackCreatorPage.h"
#include "ui_ModpackCreatorPage.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QTimer>

#include "minecraft/mod/ModpackChangelogGenerator.h"

ModpackCreatorPage::ModpackCreatorPage(MinecraftInstance* inst, QWidget* parent)
    : QWidget(parent), ui(new Ui::ModpackCreatorPage), m_inst(inst)
{
    ui->setupUi(this);

    // Load existing metadata
    auto* settings = m_inst->settings();
    bool isModpack = settings->get("ModpackCreatorEnabled").toBool();
    QString modpackName = settings->get("ModpackCreatorName").toString();
    QString modpackAuthor = settings->get("ModpackCreatorAuthor").toString();

    ui->modpackNameEdit->setText(modpackName);
    ui->modpackAuthorEdit->setText(modpackAuthor);

    updateUI();
}

ModpackCreatorPage::~ModpackCreatorPage()
{
    delete ui;
}

bool ModpackCreatorPage::apply()
{
    auto* settings = m_inst->settings();
    settings->set("ModpackCreatorName", ui->modpackNameEdit->text());
    settings->set("ModpackCreatorAuthor", ui->modpackAuthorEdit->text());
    return true;
}

bool ModpackCreatorPage::shouldDisplay() const
{
    return true;
}

void ModpackCreatorPage::retranslate()
{
    ui->retranslateUi(this);
}

void ModpackCreatorPage::on_markAsModpackBtn_clicked()
{
    auto* settings = m_inst->settings();
    bool isCurrentlyEnabled = settings->get("ModpackCreatorEnabled").toBool();

    if (isCurrentlyEnabled) {
        auto reply = QMessageBox::question(this, tr("Disable Modpack Creator"),
                                           tr("Are you sure you want to unmark this instance as a modpack?\n\n"
                                              "Version history will be preserved but can be cleared separately."),
                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply == QMessageBox::No) {
            return;
        }
    }

    settings->set("ModpackCreatorEnabled", !isCurrentlyEnabled);
    updateUI();
}

void ModpackCreatorPage::on_generateChangelogBtn_clicked()
{
    ModpackChangelogGenerator generator(m_inst);

    auto changelog = generator.generateChangelog();

    if (changelog.isEmpty() && changelog.fromVersion > 0) {
        ui->changelogOutput->setPlainText(tr("No changes detected since the last snapshot."));
    } else {
        ui->changelogOutput->setPlainText(changelog.formatAsText());
    }

    refreshVersionInfo();
}

void ModpackCreatorPage::on_copyChangelogBtn_clicked()
{
    QString text = ui->changelogOutput->toPlainText();
    if (text.isEmpty()) {
        return;
    }

    QGuiApplication::clipboard()->setText(text);

    // Brief visual feedback
    ui->copyChangelogBtn->setText(tr("Copied!"));
    QTimer::singleShot(2000, this, [this]() { ui->copyChangelogBtn->setText(tr("Copy to Clipboard")); });
}

void ModpackCreatorPage::on_clearHistoryBtn_clicked()
{
    auto reply = QMessageBox::warning(this, tr("Clear Version History"),
                                      tr("Are you sure you want to delete all version history for this modpack?\n\n"
                                         "This action cannot be undone."),
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        ModpackChangelogGenerator generator(m_inst);
        generator.clearHistory();
        ui->changelogOutput->clear();
        refreshVersionInfo();
    }
}

void ModpackCreatorPage::on_modpackNameEdit_textChanged(const QString& text)
{
    m_inst->settings()->set("ModpackCreatorName", text);
}

void ModpackCreatorPage::on_modpackAuthorEdit_textChanged(const QString& text)
{
    m_inst->settings()->set("ModpackCreatorAuthor", text);
}

void ModpackCreatorPage::updateUI()
{
    auto* settings = m_inst->settings();
    bool isModpack = settings->get("ModpackCreatorEnabled").toBool();

    // Update toggle button
    if (isModpack) {
        ui->markAsModpackBtn->setText(tr("✓ Marked as My Modpack"));
        ui->markAsModpackBtn->setToolTip(tr("Click to unmark this instance as your modpack"));
    } else {
        ui->markAsModpackBtn->setText(tr("Mark as My Modpack"));
        ui->markAsModpackBtn->setToolTip(tr("Click to mark this instance as a modpack you created"));
    }

    // Enable/disable modpack tools based on state
    ui->modpackInfoGroup->setEnabled(isModpack);
    ui->changelogGroup->setEnabled(isModpack);

    if (isModpack) {
        refreshVersionInfo();
    }
}

void ModpackCreatorPage::refreshVersionInfo()
{
    ModpackChangelogGenerator generator(m_inst);
    int snapshotCount = generator.getSnapshotCount();

    if (snapshotCount == 0) {
        ui->versionInfoLabel->setText(tr("No snapshots yet. Click \"Generate Changelog\" to create the first one."));
    } else {
        auto latest = generator.getLatestSnapshot();
        ui->versionInfoLabel->setText(
            tr("Current version: %1 | Total snapshots: %2 | Last snapshot: %3")
                .arg(latest.versionLabel)
                .arg(snapshotCount)
                .arg(latest.timestamp.toString("yyyy-MM-dd hh:mm")));
    }
}
