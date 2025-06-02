// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include <QMessageBox>

#include "ui/GuiUtil.h"
#include "ui/themes/ThemeManager.h"

#include <FileSystem.h>
#include <GZip.h>
#include <QDir>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QShortcut>
#include <QUrl>

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

LauncherLogPage::LauncherLogPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::LauncherLogPage)
    , m_model(APPLICATION->logModel)
    , m_basePath(APPLICATION->dataRoot())
    , m_logSearchPaths({ "logs" })
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
    m_proxy->setSourceModel(m_model.get());
    modelStateToUI();

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &LauncherLogPage::populateSelectLogBox);

    auto findShortcut = new QShortcut(QKeySequence(QKeySequence::Find), this);
    connect(findShortcut, &QShortcut::activated, this, &LauncherLogPage::findActivated);

    auto findNextShortcut = new QShortcut(QKeySequence(QKeySequence::FindNext), this);
    connect(findNextShortcut, &QShortcut::activated, this, &LauncherLogPage::findNextActivated);

    auto findPreviousShortcut = new QShortcut(QKeySequence(QKeySequence::FindPrevious), this);
    connect(findPreviousShortcut, &QShortcut::activated, this, &LauncherLogPage::findPreviousActivated);

    connect(ui->searchBar, &QLineEdit::returnPressed, this, &LauncherLogPage::on_findButton_clicked);
}

LauncherLogPage::~LauncherLogPage()
{
    delete ui;
}

void LauncherLogPage::modelStateToUI()
{
    if (m_model->wrapLines()) {
        ui->text->setWordWrap(true);
        ui->wrapCheckbox->setCheckState(Qt::Checked);
    } else {
        ui->text->setWordWrap(false);
        ui->wrapCheckbox->setCheckState(Qt::Unchecked);
    }
    if (m_model->colorLines()) {
        ui->text->setColorLines(true);
        ui->colorCheckbox->setCheckState(Qt::Checked);
    } else {
        ui->text->setColorLines(false);
        ui->colorCheckbox->setCheckState(Qt::Unchecked);
    }
    if (m_model->suspended()) {
        ui->trackLogCheckbox->setCheckState(Qt::Unchecked);
    } else {
        ui->trackLogCheckbox->setCheckState(Qt::Checked);
    }
}

void LauncherLogPage::UIToModelState()
{
    if (!m_model) {
        return;
    }
    m_model->setLineWrap(ui->wrapCheckbox->checkState() == Qt::Checked);
    m_model->setColorLines(ui->colorCheckbox->checkState() == Qt::Checked);
    m_model->suspend(ui->trackLogCheckbox->checkState() != Qt::Checked);
}

void LauncherLogPage::retranslate()
{
    ui->retranslateUi(this);
}

void LauncherLogPage::openedImpl()
{
    const QStringList failedPaths = m_watcher.addPaths(m_logSearchPaths);

    for (const QString& path : m_logSearchPaths) {
        if (failedPaths.contains(path))
            qDebug() << "Failed to start watching" << path;
        else
            qDebug() << "Started watching" << path;
    }

    populateSelectLogBox();
}

void LauncherLogPage::closedImpl()
{
    const QStringList failedPaths = m_watcher.removePaths(m_logSearchPaths);

    for (const QString& path : m_logSearchPaths) {
        if (failedPaths.contains(path))
            qDebug() << "Failed to stop watching" << path;
        else
            qDebug() << "Stopped watching" << path;
    }
}

void LauncherLogPage::populateSelectLogBox()
{
    const QString prevCurrentFile = m_currentFile;

    ui->selectLogBox->blockSignals(true);
    ui->selectLogBox->clear();
    ui->selectLogBox->addItem("Current logs");
    ui->selectLogBox->addItems(getPaths());
    ui->selectLogBox->blockSignals(false);

    if (!prevCurrentFile.isEmpty()) {
        const int index = ui->selectLogBox->findText(prevCurrentFile);
        if (index != -1) {
            ui->selectLogBox->blockSignals(true);
            ui->selectLogBox->setCurrentIndex(index);
            ui->selectLogBox->blockSignals(false);
            setControlsEnabled(true);
            // don't refresh file
            return;
        } else {
            setControlsEnabled(false);
        }
    } else {
        ui->selectLogBox->setCurrentIndex(0);
        setControlsEnabled(true);
    }

    on_selectLogBox_currentIndexChanged(ui->selectLogBox->currentIndex());
}

void LauncherLogPage::on_selectLogBox_currentIndexChanged(const int index)
{
    QString file;
    if (index > 0) {
        file = ui->selectLogBox->itemText(index);
    }

    if (index != 0 && (file.isEmpty() || !QFile::exists(FS::PathCombine(m_basePath, file)))) {
        m_currentFile = QString();
        ui->text->clear();
        setControlsEnabled(false);
    } else {
        m_currentFile = file;
        reload();
        setControlsEnabled(true);
    }
}

void LauncherLogPage::on_btnReload_clicked()
{
    if (m_currentFile.isEmpty()) {
        if (!m_model)
            return;
        m_model->clear();
        m_container->refreshContainer();
    } else {
        reload();
    }
}

void LauncherLogPage::reload()
{
    if (m_currentFile.isEmpty()) {
        m_model = APPLICATION->logModel;
        m_proxy->setSourceModel(m_model.get());
        ui->text->setModel(m_proxy);
        ui->text->scrollToBottom();
        UIToModelState();
        setControlsEnabled(true);
        return;
    }

    QFile file(FS::PathCombine(m_basePath, m_currentFile));
    if (!file.open(QFile::ReadOnly)) {
        setControlsEnabled(false);
        ui->btnReload->setEnabled(true);  // allow reload
        m_currentFile = QString();
        QMessageBox::critical(this, tr("Error"), tr("Unable to open %1 for reading: %2").arg(m_currentFile, file.errorString()));
    } else {
        auto setPlainText = [this](const QString& text) {
            QTextDocument* doc = ui->text->document();
            doc->setDefaultFont(m_proxy->getFont());
            ui->text->setPlainText(text);
        };
        auto showTooBig = [setPlainText, &file]() {
            setPlainText(tr("The file (%1) is too big. You may want to open it in a viewer optimized "
                            "for large files.")
                             .arg(file.fileName()));
        };
        if (file.size() > (1024ll * 1024ll * 12ll)) {
            showTooBig();
            return;
        }
        MessageLevel::Enum last = MessageLevel::Unknown;

        auto handleLine = [this, &last](QString line) {
            if (line.isEmpty())
                return false;
            if (line.back() == '\n')
                line = line.remove(line.size() - 1, 1);
            QString lineTemp = line;  // don't edit out the time and level for clarity
            MessageLevel::Enum level = MessageLevel::fromLauncherLine(lineTemp);

            last = level;
            m_model->append(level, line);
            return m_model->isOverFlow();
        };

        // Try to determine a level for each line
        ui->text->clear();
        ui->text->setModel(nullptr);
        m_model.reset(new LogModel(this));
        m_model->setMaxLines(APPLICATION->getConsoleMaxLines());
        m_model->setStopOnOverflow(APPLICATION->shouldStopOnConsoleOverflow());
        m_model->setOverflowMessage(tr("Cannot display this log since the log length surpassed %1 lines.").arg(m_model->getMaxLines()));
        m_model->clear();
        if (file.fileName().endsWith(".gz")) {
            QString line;
            auto error = GZip::readGzFileByBlocks(&file, [&line, handleLine](const QByteArray& d) {
                auto block = d;
                int newlineIndex = block.indexOf('\n');
                while (newlineIndex != -1) {
                    line += QString::fromUtf8(block).left(newlineIndex);
                    block.remove(0, newlineIndex + 1);
                    if (handleLine(line)) {
                        line.clear();
                        return false;
                    }
                    line.clear();
                    newlineIndex = block.indexOf('\n');
                }
                line += QString::fromUtf8(block);
                return true;
            });
            if (!error.isEmpty()) {
                setPlainText(tr("The file (%1) encountered an error when reading: %2.").arg(file.fileName(), error));
                return;
            } else if (!line.isEmpty()) {
                handleLine(line);
            }
        } else {
            while (!file.atEnd() && !handleLine(QString::fromUtf8(file.readLine()))) {
            }
        }
        m_proxy->setSourceModel(m_model.get());
        ui->text->setModel(m_proxy);
        ui->text->scrollToBottom();
        UIToModelState();
        setControlsEnabled(true);
    }
}

void LauncherLogPage::on_btnPaste_clicked()
{
    GuiUtil::uploadPaste(m_currentFile, ui->text->toPlainText(), this);
}

void LauncherLogPage::on_btnCopy_clicked()
{
    GuiUtil::setClipboardText(ui->text->toPlainText());
}

void LauncherLogPage::on_btnBottom_clicked()
{
    ui->text->scrollToBottom();
}

void LauncherLogPage::on_trackLogCheckbox_clicked(bool checked)
{
    if (!m_model)
        return;
    m_model->suspend(!checked);
}

void LauncherLogPage::on_btnDelete_clicked()
{
    if (m_currentFile.isEmpty()) {
        setControlsEnabled(false);
        return;
    }
    if (QMessageBox::question(this, tr("Confirm Deletion"),
                              tr("You are about to delete \"%1\".\n"
                                 "This may be permanent and it will be gone from the logs folder.\n\n"
                                 "Are you sure?")
                                  .arg(m_currentFile),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
        return;
    }
    QFile file(FS::PathCombine(m_basePath, m_currentFile));

    if (FS::trash(file.fileName())) {
        return;
    }

    if (!file.remove()) {
        QMessageBox::critical(this, tr("Error"), tr("Unable to delete %1: %2").arg(m_currentFile, file.errorString()));
    }
}

void LauncherLogPage::on_btnClean_clicked()
{
    auto toDelete = getPaths();
    if (toDelete.isEmpty()) {
        return;
    }
    QMessageBox* messageBox = new QMessageBox(this);
    messageBox->setWindowTitle(tr("Confirm Cleanup"));
    if (toDelete.size() > 5) {
        messageBox->setText(tr("Are you sure you want to delete all log files?"));
        messageBox->setDetailedText(toDelete.join('\n'));
    } else {
        messageBox->setText(tr("Are you sure you want to delete all these files?\n%1").arg(toDelete.join('\n')));
    }
    messageBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    messageBox->setDefaultButton(QMessageBox::Ok);
    messageBox->setTextInteractionFlags(Qt::TextSelectableByMouse);
    messageBox->setIcon(QMessageBox::Question);
    messageBox->setTextInteractionFlags(Qt::TextBrowserInteraction);

    if (messageBox->exec() != QMessageBox::Ok) {
        return;
    }
    QStringList failed;
    for (auto item : toDelete) {
        QString absolutePath = FS::PathCombine(m_basePath, item);
        QFile file(absolutePath);
        qDebug() << "Deleting log" << absolutePath;
        if (FS::trash(file.fileName())) {
            continue;
        }
        if (!file.remove()) {
            failed.push_back(item);
        }
    }
    if (!failed.empty()) {
        QMessageBox* messageBoxFailure = new QMessageBox(this);
        messageBoxFailure->setWindowTitle(tr("Error"));
        if (failed.size() > 5) {
            messageBoxFailure->setText(tr("Couldn't delete some files!"));
            messageBoxFailure->setDetailedText(failed.join('\n'));
        } else {
            messageBoxFailure->setText(tr("Couldn't delete some files:\n%1").arg(failed.join('\n')));
        }
        messageBoxFailure->setStandardButtons(QMessageBox::Ok);
        messageBoxFailure->setDefaultButton(QMessageBox::Ok);
        messageBoxFailure->setTextInteractionFlags(Qt::TextSelectableByMouse);
        messageBoxFailure->setIcon(QMessageBox::Critical);
        messageBoxFailure->setTextInteractionFlags(Qt::TextBrowserInteraction);
        messageBoxFailure->exec();
    }
}

void LauncherLogPage::on_wrapCheckbox_clicked(bool checked)
{
    ui->text->setWordWrap(checked);
    if (!m_model)
        return;
    m_model->setLineWrap(checked);
    ui->text->scrollToBottom();
}

void LauncherLogPage::on_colorCheckbox_clicked(bool checked)
{
    ui->text->setColorLines(checked);
    if (!m_model)
        return;
    m_model->setColorLines(checked);
    ui->text->scrollToBottom();
}

void LauncherLogPage::setControlsEnabled(const bool enabled)
{
    if (!m_currentFile.isEmpty()) {
        ui->btnReload->setText("&Reload");
        ui->btnReload->setToolTip("Reload the contents of the log from the disk");
        ui->btnDelete->setEnabled(enabled);
        ui->btnClean->setEnabled(enabled);
        ui->trackLogCheckbox->setEnabled(false);
    } else {
        ui->btnReload->setText("Clear");
        ui->btnReload->setToolTip("Clear the log");
        ui->btnDelete->setEnabled(false);
        ui->btnClean->setEnabled(false);
        ui->trackLogCheckbox->setEnabled(enabled);
    }
    ui->btnReload->setEnabled(enabled);
    ui->btnCopy->setEnabled(enabled);
    ui->btnPaste->setEnabled(enabled);
    ui->text->setEnabled(enabled);
}

QStringList LauncherLogPage::getPaths()
{
    QDir baseDir(m_basePath);

    QStringList result;

    for (QString searchPath : m_logSearchPaths) {
        QDir searchDir(searchPath);

        QStringList filters{ "*.log", "*.log.gz" };

        if (searchPath != m_basePath)
            filters.append("*.txt");

        QStringList entries = searchDir.entryList(filters, QDir::Files | QDir::Readable, QDir::SortFlag::Time);

        for (const QString& name : entries)
            result.append(baseDir.relativeFilePath(searchDir.filePath(name)));
    }

    return result;
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
