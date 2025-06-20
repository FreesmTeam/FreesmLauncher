/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CustomLoginDialog.h"
#include "ui_CustomLoginDialog.h"

#include <QPushButton>
#include <QUrl>

CustomLoginDialog::CustomLoginDialog(QWidget* parent) : QDialog(parent), ui(new Ui::CustomLoginDialog)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CustomLoginDialog::~CustomLoginDialog()
{
    delete ui;
}

// Stage 1: User interaction
void CustomLoginDialog::accept()
{
    QUrl url = ui->authUrlTextBox->text();
    if (!url.isValid()) {
        emit onTaskFailed(tr("Provided URL isn't valid"));
        return;
    }
    if (url.scheme() != "https") {
        emit onTaskFailed(tr("Use encrypted HTTP protocol (https://)"));
        return;
    }

    setUserInputsEnabled(false);
    ui->progressBar->setVisible(true);

    // Setup the login task and start it
    m_account = CustomAccount::createCustom(ui->userTextBox->text(), url.toString(QUrl::StripTrailingSlash));
    m_loginTask = m_account->login(ui->passTextBox->text());
    connect(m_loginTask.get(), &Task::failed, this, &CustomLoginDialog::onTaskFailed);
    connect(m_loginTask.get(), &Task::succeeded, this, &CustomLoginDialog::onTaskSucceeded);
    connect(m_loginTask.get(), &Task::status, this, &CustomLoginDialog::onTaskStatus);
    connect(m_loginTask.get(), &Task::progress, this, &CustomLoginDialog::onTaskProgress);
    m_loginTask->start();
}

void CustomLoginDialog::setUserInputsEnabled(bool enable)
{
    ui->authUrlTextBox->setEnabled(enable);
    ui->userTextBox->setEnabled(enable);
    ui->passTextBox->setEnabled(enable);
    ui->buttonBox->setEnabled(enable);
}

// Enable the OK button only when both textboxes contain something.
void CustomLoginDialog::on_authUrlTextBox_textEdited(const QString& newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!newText.isEmpty() && !ui->userTextBox->text().isEmpty() && !ui->passTextBox->text().isEmpty());
}
void CustomLoginDialog::on_userTextBox_textEdited(const QString& newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!newText.isEmpty() && !ui->passTextBox->text().isEmpty() && !ui->authUrlTextBox->text().isEmpty());
}
void CustomLoginDialog::on_passTextBox_textEdited(const QString& newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!newText.isEmpty() && !ui->userTextBox->text().isEmpty() && !ui->authUrlTextBox->text().isEmpty());
}

void CustomLoginDialog::onTaskFailed(const QString& reason)
{
    // Set message
    auto lines = reason.split('\n');
    QString processed;
    for (auto line : lines) {
        if (line.size()) {
            processed += "<font color='red'>" + line + "</font><br />";
        } else {
            processed += "<br />";
        }
    }
    ui->label->setText(processed);

    // Re-enable user-interaction
    setUserInputsEnabled(true);
    ui->progressBar->setVisible(false);
}

void CustomLoginDialog::onTaskSucceeded()
{
    QDialog::accept();
}

void CustomLoginDialog::onTaskStatus(const QString& status)
{
    ui->label->setText(status);
}

void CustomLoginDialog::onTaskProgress(qint64 current, qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(current);
}

// Public interface
CustomAccountPtr CustomLoginDialog::newAccount(QWidget* parent, QString msg)
{
    CustomLoginDialog dlg(parent);
    dlg.ui->label->setText(msg);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.m_account;
    }
    return nullptr;
}
