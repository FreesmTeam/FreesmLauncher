// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2024 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (c) 2025 Yihe Li <winmikedows@hotmail.com>
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

#include "LauncherLogPage.h"
#include "ui_LauncherLogPage.h"

#include "Application.h"

#include <QIdentityProxyModel>
#include <QScrollBar>
#include <QShortcut>

#include "launch/LaunchTask.h"
#include "settings/Setting.h"

#include "ui/GuiUtil.h"
#include "ui/themes/ThemeManager.h"

#include <BuildConfig.h>

QVariant LogFormatProxyModel::data(const QModelIndex& index, int role) const
{
    const LogColors& colors = APPLICATION->themeManager()->getLogColors();

    switch (role) {
        case Qt::FontRole:
            return m_font;
        case Qt::ForegroundRole: {
            auto level = static_cast<MessageLevel::Enum>(QIdentityProxyModel::data(index, LogModel::LevelRole).toInt());
            QColor result = colors.foreground.value(level);

            if (result.isValid())
                return result;

            break;
        }
        case Qt::BackgroundRole: {
            auto level = static_cast<MessageLevel::Enum>(QIdentityProxyModel::data(index, LogModel::LevelRole).toInt());
            QColor result = colors.background.value(level);

            if (result.isValid())
                return result;

            break;
        }
    }

    return QIdentityProxyModel::data(index, role);
}

QModelIndex LogFormatProxyModel::find(const QModelIndex& start, const QString& value, bool reverse) const
{
    QModelIndex parentIndex = parent(start);
    auto compare = [this, start, parentIndex, value](int r) -> QModelIndex {
        QModelIndex idx = index(r, start.column(), parentIndex);
        if (!idx.isValid() || idx == start) {
            return QModelIndex();
        }
        QVariant v = data(idx, Qt::DisplayRole);
        QString t = v.toString();
        if (t.contains(value, Qt::CaseInsensitive))
            return idx;
        return QModelIndex();
    };
    if (reverse) {
        int from = start.row();
        int to = 0;

        for (int i = 0; i < 2; ++i) {
            for (int r = from; (r >= to); --r) {
                auto idx = compare(r);
                if (idx.isValid())
                    return idx;
            }
            // prepare for the next iteration
            from = rowCount() - 1;
            to = start.row();
        }
    } else {
        int from = start.row();
        int to = rowCount(parentIndex);

        for (int i = 0; i < 2; ++i) {
            for (int r = from; (r < to); ++r) {
                auto idx = compare(r);
                if (idx.isValid())
                    return idx;
            }
            // prepare for the next iteration
            from = 0;
            to = start.row();
        }
    }
    return QModelIndex();
}

LauncherLogPage::LauncherLogPage(QWidget* parent) : QWidget(parent), ui(new Ui::LauncherLogPage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();

    m_proxy = new LogFormatProxyModel(this);

    // set up fonts in the log proxy
    {
        QString fontFamily = APPLICATION->settings()->get("ConsoleFont").toString();
        bool conversionOk = false;
        int fontSize = APPLICATION->settings()->get("ConsoleFontSize").toInt(&conversionOk);
        if (!conversionOk) {
            fontSize = 11;
        }
        m_proxy->setFont(QFont(fontFamily, fontSize));
    }

    ui->text->setModel(m_proxy);
    m_proxy->setSourceModel(APPLICATION->logModel.get());
    modelStateToUI();

    auto findShortcut = new QShortcut(QKeySequence(QKeySequence::Find), this);
    connect(findShortcut, SIGNAL(activated()), SLOT(findActivated()));
    auto findNextShortcut = new QShortcut(QKeySequence(QKeySequence::FindNext), this);
    connect(findNextShortcut, SIGNAL(activated()), SLOT(findNextActivated()));
    connect(ui->searchBar, SIGNAL(returnPressed()), SLOT(on_findButton_clicked()));
    auto findPreviousShortcut = new QShortcut(QKeySequence(QKeySequence::FindPrevious), this);
    connect(findPreviousShortcut, SIGNAL(activated()), SLOT(findPreviousActivated()));
}

LauncherLogPage::~LauncherLogPage()
{
    delete ui;
}

void LauncherLogPage::modelStateToUI()
{
    if (APPLICATION->logModel->wrapLines()) {
        ui->text->setWordWrap(true);
        ui->wrapCheckbox->setCheckState(Qt::Checked);
    } else {
        ui->text->setWordWrap(false);
        ui->wrapCheckbox->setCheckState(Qt::Unchecked);
    }
    if (APPLICATION->logModel->colorLines()) {
        ui->text->setColorLines(true);
        ui->colorCheckbox->setCheckState(Qt::Checked);
    } else {
        ui->text->setColorLines(false);
        ui->colorCheckbox->setCheckState(Qt::Unchecked);
    }
    if (APPLICATION->logModel->suspended()) {
        ui->trackLogCheckbox->setCheckState(Qt::Unchecked);
    } else {
        ui->trackLogCheckbox->setCheckState(Qt::Checked);
    }
}

void LauncherLogPage::UIToModelState()
{
    if (!APPLICATION->logModel) {
        return;
    }
    APPLICATION->logModel->setLineWrap(ui->wrapCheckbox->checkState() == Qt::Checked);
    APPLICATION->logModel->setColorLines(ui->colorCheckbox->checkState() == Qt::Checked);
    APPLICATION->logModel->suspend(ui->trackLogCheckbox->checkState() != Qt::Checked);
}

void LauncherLogPage::on_btnPaste_clicked()
{
    if (!APPLICATION->logModel)
        return;

    // FIXME: turn this into a proper task and move the upload logic out of GuiUtil!
    APPLICATION->logModel->append(MessageLevel::Launcher,
                                  QString("Log upload triggered at: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)));
    auto url = GuiUtil::uploadPaste(tr("Launcher Log"), APPLICATION->logModel->toPlainText(), this);
    if (!url.has_value()) {
        APPLICATION->logModel->append(MessageLevel::Error, QString("Log upload canceled"));
    } else if (url->isNull()) {
        APPLICATION->logModel->append(MessageLevel::Error, QString("Log upload failed!"));
    } else {
        APPLICATION->logModel->append(MessageLevel::Launcher, QString("Log uploaded to: %1").arg(url.value()));
    }
}

void LauncherLogPage::on_btnCopy_clicked()
{
    if (!APPLICATION->logModel)
        return;
    APPLICATION->logModel->append(MessageLevel::Launcher,
                                  QString("Clipboard copy at: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)));
    GuiUtil::setClipboardText(APPLICATION->logModel->toPlainText());
}

void LauncherLogPage::on_btnClear_clicked()
{
    if (!APPLICATION->logModel)
        return;
    APPLICATION->logModel->clear();
    m_container->refreshContainer();
}

void LauncherLogPage::on_btnBottom_clicked()
{
    ui->text->scrollToBottom();
}

void LauncherLogPage::on_trackLogCheckbox_clicked(bool checked)
{
    if (!APPLICATION->logModel)
        return;
    APPLICATION->logModel->suspend(!checked);
}

void LauncherLogPage::on_wrapCheckbox_clicked(bool checked)
{
    ui->text->setWordWrap(checked);
    if (!APPLICATION->logModel)
        return;
    APPLICATION->logModel->setLineWrap(checked);
}

void LauncherLogPage::on_colorCheckbox_clicked(bool checked)
{
    ui->text->setColorLines(checked);
    if (!APPLICATION->logModel)
        return;
    APPLICATION->logModel->setColorLines(checked);
}

void LauncherLogPage::on_findButton_clicked()
{
    auto modifiers = QApplication::keyboardModifiers();
    bool reverse = modifiers & Qt::ShiftModifier;
    ui->text->findNext(ui->searchBar->text(), reverse);
}

void LauncherLogPage::findNextActivated()
{
    ui->text->findNext(ui->searchBar->text(), false);
}

void LauncherLogPage::findPreviousActivated()
{
    ui->text->findNext(ui->searchBar->text(), true);
}

void LauncherLogPage::findActivated()
{
    // focus the search bar if it doesn't have focus
    if (!ui->searchBar->hasFocus()) {
        ui->searchBar->setFocus();
        ui->searchBar->selectAll();
    }
}

void LauncherLogPage::retranslate()
{
    ui->retranslateUi(this);
}
