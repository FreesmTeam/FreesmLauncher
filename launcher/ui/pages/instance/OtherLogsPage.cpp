// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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

#include "OtherLogsPage.h"
#include "ui_OtherLogsPage.h"

#include <QMessageBox>

#include "ui/GuiUtil.h"

#include <FileSystem.h>
#include <GZip.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qfilesystemwatcher.h>
#include <qurl.h>
#include <QShortcut>

OtherLogsPage::OtherLogsPage(InstancePtr instance, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::OtherLogsPage)
    , m_instance(instance)
    , m_basePath(instance->gameRoot())
    , m_logSearchPaths(instance->getLogFileSearchPaths())
    , m_model(new LogModel(this))
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

    m_model->setMaxLines(m_instance->getConsoleMaxLines());
    m_model->setStopOnOverflow(m_instance->shouldStopOnConsoleOverflow());
    m_model->setOverflowMessage(tr("Cannot display this log since the log length surpassed %1 lines.").arg(m_model->getMaxLines()));
    m_proxy->setSourceModel(m_model.get());

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &OtherLogsPage::populateSelectLogBox);

    auto findShortcut = new QShortcut(QKeySequence(QKeySequence::Find), this);
    connect(findShortcut, &QShortcut::activated, this, &OtherLogsPage::findActivated);

    auto findNextShortcut = new QShortcut(QKeySequence(QKeySequence::FindNext), this);
    connect(findNextShortcut, &QShortcut::activated, this, &OtherLogsPage::findNextActivated);

    auto findPreviousShortcut = new QShortcut(QKeySequence(QKeySequence::FindPrevious), this);
    connect(findPreviousShortcut, &QShortcut::activated, this, &OtherLogsPage::findPreviousActivated);

    connect(ui->searchBar, &QLineEdit::returnPressed, this, &OtherLogsPage::on_findButton_clicked);
}

OtherLogsPage::~OtherLogsPage()
{
    delete ui;
}

void OtherLogsPage::retranslate()
{
    ui->retranslateUi(this);
}

void OtherLogsPage::openedImpl()
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

void OtherLogsPage::closedImpl()
{
    const QStringList failedPaths = m_watcher.removePaths(m_logSearchPaths);

    for (const QString& path : m_logSearchPaths) {
        if (failedPaths.contains(path))
            qDebug() << "Failed to stop watching" << path;
        else
            qDebug() << "Stopped watching" << path;
    }
}

void OtherLogsPage::populateSelectLogBox()
{
    const QString prevCurrentFile = m_currentFile;

    ui->selectLogBox->blockSignals(true);
    ui->selectLogBox->clear();
    ui->selectLogBox->addItems(getPaths());
    ui->selectLogBox->blockSignals(false);

    if (!prevCurrentFile.isEmpty()) {
        const int index = ui->selectLogBox->findText(prevCurrentFile);
        if (index != -1) {
            ui->selectLogBox->setCurrentIndex(index);
            setControlsEnabled(true);
            return;
        } else {
            setControlsEnabled(false);
        }
    }

    on_selectLogBox_currentIndexChanged(ui->selectLogBox->currentIndex());
}

void OtherLogsPage::on_selectLogBox_currentIndexChanged(const int index)
{
    QString file;
    if (index != -1) {
        file = ui->selectLogBox->itemText(index);
    }

    if (file.isEmpty() || !QFile::exists(FS::PathCombine(m_basePath, file))) {
        m_currentFile = QString();
        ui->text->clear();
        setControlsEnabled(false);
    } else {
        m_currentFile = file;
        on_btnReload_clicked();
        setControlsEnabled(true);
    }
}

class ReadLineAbstract {
   public:
    ReadLineAbstract(QFile* file) : m_file(file)
    {
        if (file->fileName().endsWith(".gz"))
            m_gz = new GZipStream(file);
    }
    ~ReadLineAbstract() { delete m_gz; }

    QString readLine()
    {
        if (!m_gz)
            return QString::fromUtf8(m_file->readLine());
        QString line;
        for (;;) {
            if (!m_decodedData.isEmpty()) {
                int newlineIndex = m_decodedData.indexOf('\n');
                if (newlineIndex != -1) {
                    line += QString::fromUtf8(m_decodedData).left(newlineIndex);
                    m_decodedData.remove(0, newlineIndex + 1);
                    return line;
                }

                line += QString::fromUtf8(m_decodedData);
                m_decodedData.clear();
            }

            if (!m_gz->unzipBlockByBlock(m_decodedData)) {  // If error occurs during unzipping
                m_decodedData.clear();
                return QObject::tr("The content of the file(%1) could not be decoded.").arg(m_file->fileName());
            }
        }
    }

    bool done() { return m_gz ? m_decodedData.isEmpty() : m_file->atEnd(); }

   private:
    QFile* m_file;
    GZipStream* m_gz = nullptr;
    QByteArray m_decodedData;
};

void OtherLogsPage::on_btnReload_clicked()
{
    if (m_currentFile.isEmpty()) {
        setControlsEnabled(false);
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

        ReadLineAbstract stream(&file);

        // Try to determine a level for each line
        ui->text->clear();
        ui->text->setModel(nullptr);
        m_model->clear();
        auto line = stream.readLine();
        while (!stream.done()) {  // just read until the model is full or the file ended
            if (line.back() == '\n')
                line = line.remove(line.size() - 1, 1);
            MessageLevel::Enum level = MessageLevel::Unknown;

            // if the launcher part set a log level, use it
            auto innerLevel = MessageLevel::fromLine(line);
            if (innerLevel != MessageLevel::Unknown) {
                level = innerLevel;
            }

            // If the level is still undetermined, guess level
            if (level == MessageLevel::StdErr || level == MessageLevel::StdOut || level == MessageLevel::Unknown) {
                level = m_instance->guessLevel(line, level);
            }

            m_model->append(level, line);
            if (m_model->isOverFlow())
                break;

            line = stream.readLine();
        }
        ui->text->setModel(m_proxy);
        ui->text->scrollToBottom();
    }
}

void OtherLogsPage::on_btnPaste_clicked()
{
    GuiUtil::uploadPaste(m_currentFile, ui->text->toPlainText(), this);
}

void OtherLogsPage::on_btnCopy_clicked()
{
    GuiUtil::setClipboardText(ui->text->toPlainText());
}

void OtherLogsPage::on_btnBottom_clicked()
{
    ui->text->scrollToBottom();
}

void OtherLogsPage::on_btnDelete_clicked()
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

void OtherLogsPage::on_btnClean_clicked()
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

void OtherLogsPage::on_wrapCheckbox_clicked(bool checked)
{
    ui->text->setWordWrap(checked);
    if (!m_model)
        return;
    m_model->setLineWrap(checked);
    ui->text->scrollToBottom();
}

void OtherLogsPage::on_colorCheckbox_clicked(bool checked)
{
    ui->text->setColorLines(checked);
    if (!m_model)
        return;
    m_model->setColorLines(checked);
    ui->text->scrollToBottom();
}

void OtherLogsPage::setControlsEnabled(const bool enabled)
{
    ui->btnReload->setEnabled(enabled);
    ui->btnDelete->setEnabled(enabled);
    ui->btnCopy->setEnabled(enabled);
    ui->btnPaste->setEnabled(enabled);
    ui->text->setEnabled(enabled);
    ui->btnClean->setEnabled(enabled);
}

QStringList OtherLogsPage::getPaths()
{
    QDir baseDir(m_basePath);

    QStringList result;

    for (QString searchPath : m_logSearchPaths) {
        QDirIterator iterator(searchPath, QDir::Files | QDir::Readable);

        while (iterator.hasNext()) {
            const QString name = iterator.next();

            if (!name.endsWith(".log") && !name.endsWith(".log.gz"))
                continue;

            result.append(baseDir.relativeFilePath(name));
        }
    }

    return result;
}

void OtherLogsPage::on_findButton_clicked()
{
    auto modifiers = QApplication::keyboardModifiers();
    bool reverse = modifiers & Qt::ShiftModifier;
    ui->text->findNext(ui->searchBar->text(), reverse);
}

void OtherLogsPage::findNextActivated()
{
    ui->text->findNext(ui->searchBar->text(), false);
}

void OtherLogsPage::findPreviousActivated()
{
    ui->text->findNext(ui->searchBar->text(), true);
}

void OtherLogsPage::findActivated()
{
    // focus the search bar if it doesn't have focus
    if (!ui->searchBar->hasFocus()) {
        ui->searchBar->setFocus();
        ui->searchBar->selectAll();
    }
}
