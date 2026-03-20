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

#include <QMessageBox>
#include <QPushButton>
#include <QUrl>

#include "Application.h"
#include "UrlUtils.h"
#include "net/Download.h"

namespace {
QUrl formUrl(QString userInput, QString& errorString)
{
    errorString.clear();

    userInput = userInput.trimmed();

    bool httpScheme = userInput.startsWith("http://", Qt::CaseInsensitive);
    bool httpsScheme = userInput.startsWith("https://", Qt::CaseInsensitive);

    if (userInput.contains("://") && !httpsScheme && !httpScheme) {
        errorString = QObject::tr("Invalid URL scheme");
        return {};
    }

    QUrl deducedUrl = QUrl::fromUserInput(userInput);

    if (!deducedUrl.isValid() || deducedUrl.isLocalFile() || deducedUrl.host().isEmpty()) {
        errorString = QObject::tr("Invalid URL");
        return {};
    }

    if (!httpsScheme && !httpScheme) {
        deducedUrl.setScheme("https");
    }

    return deducedUrl;
}
}  // namespace

CustomLoginDialog::CustomLoginDialog(QWidget* parent) : QDialog(parent), ui(new Ui::CustomLoginDialog)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui->userTextBox, &QLineEdit::textChanged, this, &CustomLoginDialog::onTextBoxesChanged);
    connect(ui->passTextBox, &QLineEdit::textChanged, this, &CustomLoginDialog::onTextBoxesChanged);
    connect(ui->authUrlTextBox, &QLineEdit::textChanged, this, &CustomLoginDialog::onTextBoxesChanged);
    connect(ui->loginUrlTextBox, &QLineEdit::textChanged, this, &CustomLoginDialog::onTextBoxesChanged);
    connect(ui->refreshUrlTextBox, &QLineEdit::textChanged, this, &CustomLoginDialog::onTextBoxesChanged);

    connect(ui->authUrlTextBox, &QLineEdit::textChanged, this, &CustomLoginDialog::onAuthUrlTextBoxChanged);
}

CustomLoginDialog::~CustomLoginDialog()
{
    delete ui;
}

// Stage 1: User interaction
void CustomLoginDialog::accept()
{
    QString errorString;

    const QUrl url(formUrl(ui->authUrlTextBox->text(), errorString));
    if (!url.isValid()) {
        emit onTaskFailed(errorString);
        return;
    }

    m_loginUrl = url;

    setUserInputsEnabled(false);
    ui->progressBar->setVisible(true);

    m_requestTask = Net::Download::makeByteArray(m_loginUrl, std::make_shared<QByteArray>());
    m_requestTask->setNetwork(APPLICATION->network());

    connect(m_requestTask.get(), &Task::finished, this, &CustomLoginDialog::onUrlResolving);

    m_requestTask->start();
}

void CustomLoginDialog::onUrlResolving()
{
    disconnect(m_requestTask.get(), &Task::finished, this, &CustomLoginDialog::onUrlResolving);

    if (m_requestTask->error() != QNetworkReply::NoError) {
        emit onTaskFailed(m_requestTask->errorString());
        return;
    }

    m_resolvedUrl = m_loginUrl;

    // modify url if header say so
    auto headers = m_requestTask->getRawHeaders();
    if (const auto it =
            std::find_if(headers.begin(), headers.end(),
                         [](const auto& pair) { return QString::fromUtf8(pair.first).toLower() == "x-authlib-injector-api-location"; });
        it != headers.end()) {
        const QUrl location = QString::fromUtf8(it->second);
        if (location.isRelative()) {
            m_resolvedUrl = m_requestTask->url().resolved(location);
        } else {
            m_resolvedUrl = location;
        }
    }

    bool shouldContinue = showWarning();
    if (!shouldContinue) {
        emit onTaskFailed(tr("Aborted"));
        return;
    }

    // Setup the login task and start it
    m_account = CustomAccount::createCustom(ui->userTextBox->text(), m_resolvedUrl.toString(QUrl::StripTrailingSlash),
                                            ui->loginUrlTextBox->text(), ui->refreshUrlTextBox->text());
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
    ui->loginUrlTextBox->setEnabled(enable);
    ui->refreshUrlTextBox->setEnabled(enable);
    ui->userTextBox->setEnabled(enable);
    ui->passTextBox->setEnabled(enable);
    ui->buttonBox->setEnabled(enable);
}

bool CustomLoginDialog::showWarning()
{
    QString text = tr("You entered:\n%1\n"
                      "Your login credentials will be sent to:\n%2\n"
                      "Do you want to continue?")
                       .arg(m_loginUrl.toString(), m_resolvedUrl.toString());

    if (UrlUtils::isUnsafe(m_loginUrl) || UrlUtils::isUnsafe(m_resolvedUrl)) {
        text.prepend(tr("Please note that http:// is not secure, and your login credentials may be intercepted.\n"));
    }

    auto answer = QMessageBox::question(this, tr("Warning"), text);

    return answer == QMessageBox::Yes;
}

// Enable the OK button only when both textboxes contain something.
void CustomLoginDialog::onTextBoxesChanged()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!ui->userTextBox->text().isEmpty() && !ui->passTextBox->text().isEmpty() && !ui->authUrlTextBox->text().isEmpty() &&
                     !ui->loginUrlTextBox->text().isEmpty() && !ui->refreshUrlTextBox->text().isEmpty());
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

void CustomLoginDialog::onAuthUrlTextBoxChanged()
{
    ui->loginUrlTextBox->setText("/authserver/authenticate");
    ui->refreshUrlTextBox->setText("/authserver/refresh");
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
