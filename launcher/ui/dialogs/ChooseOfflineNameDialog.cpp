// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 Octol1ttle <l1ttleofficial@outlook.com>
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

#include "ChooseOfflineNameDialog.h"

#include <QPushButton>
#include <QRegularExpression>
#include <QRandomGenerator>

#include "ui_ChooseOfflineNameDialog.h"

ChooseOfflineNameDialog::ChooseOfflineNameDialog(const QString& message, QWidget* parent)
    : QDialog(parent), ui(new Ui::ChooseOfflineNameDialog)
{
    ui->setupUi(this);
    ui->label->setText(message);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    const QRegularExpression usernameRegExp("^[A-Za-z0-9_]{3,16}$");
    m_usernameValidator = new QRegularExpressionValidator(usernameRegExp, this);
    ui->usernameTextBox->setValidator(m_usernameValidator);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

ChooseOfflineNameDialog::~ChooseOfflineNameDialog()
{
    delete ui;
}

QString ChooseOfflineNameDialog::getUsername() const
{
    return ui->usernameTextBox->text();
}

void ChooseOfflineNameDialog::setUsername(const QString& username) const
{
    ui->usernameTextBox->setText(username);
    updateAcceptAllowed(username);
}

void ChooseOfflineNameDialog::updateAcceptAllowed(const QString& username) const
{
    const bool allowed = ui->allowInvalidUsernames->isChecked() ? !username.isEmpty() : ui->usernameTextBox->hasAcceptableInput();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(allowed);
}

void ChooseOfflineNameDialog::on_usernameTextBox_textEdited(const QString& newText) const
{
    updateAcceptAllowed(newText);
}

void ChooseOfflineNameDialog::on_allowInvalidUsernames_checkStateChanged(const Qt::CheckState checkState) const
{
    ui->usernameTextBox->setValidator(checkState == Qt::Checked ? nullptr : m_usernameValidator);
    updateAcceptAllowed(getUsername());
}

void ChooseOfflineNameDialog::on_randomCharUser_clicked()
{
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    const int randomStringLength = 14; 
    QString randomString;
    for(int i=0; i<randomStringLength; ++i)
    {
       int index = QRandomGenerator::global()->generate() % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
    }
    ui->usernameTextBox->setText(randomString);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void ChooseOfflineNameDialog::on_randomFullUser_clicked()
{
    //todo: maybe make this words configurable and not hardcoded, smh.
    QList<QString> PossibleUsernameHalves = {"Cookie", "Clicker", "Licker", "Lenny", "Super", "Sakupen", "Sonic", "Geometry", "Mining", "Chicken", "Sculpted", "Random", "Painted", "Fainted", "MadeIn", "Chinese", "Bing", "Hell", "Circles", "Wave", "Dash", "Crafting", "Smelting", "Jockey", "Vase", "Heaven", "Pudding", "Chilling"};
    int UsernameArrayLength = PossibleUsernameHalves.count();
    //int indexPossibleHalf = QRandomGenerator::global()->bounded(22);
    int indexFirstPossibleHalf = QRandomGenerator::global()->bounded(UsernameArrayLength);
    int indexSecondPossibleHalf = QRandomGenerator::global()->bounded(UsernameArrayLength);
    QString GeneratedUsername = PossibleUsernameHalves[indexFirstPossibleHalf] + PossibleUsernameHalves[indexSecondPossibleHalf];
    ui->usernameTextBox->setText(GeneratedUsername);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

