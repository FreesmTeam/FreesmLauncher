#include "OfflineLoginDialog.h"
#include "ui_OfflineLoginDialog.h"
#include <QRandomGenerator>
#include <QtWidgets/QPushButton>

OfflineLoginDialog::OfflineLoginDialog(QWidget* parent) : QDialog(parent), ui(new Ui::OfflineLoginDialog)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

OfflineLoginDialog::~OfflineLoginDialog()
{
    delete ui;
}

// Stage 1: User interaction
void OfflineLoginDialog::accept()
{
    setUserInputsEnabled(false);
    ui->progressBar->setVisible(true);

    // Setup the login task and start it
    m_account = OfflineAccount::createOffline(ui->userTextBox->text());
    m_loginTask = m_account->login();
    connect(m_loginTask.get(), &Task::failed, this, &OfflineLoginDialog::onTaskFailed);
    connect(m_loginTask.get(), &Task::succeeded, this, &OfflineLoginDialog::onTaskSucceeded);
    connect(m_loginTask.get(), &Task::status, this, &OfflineLoginDialog::onTaskStatus);
    connect(m_loginTask.get(), &Task::progress, this, &OfflineLoginDialog::onTaskProgress);
    m_loginTask->start();
}

void OfflineLoginDialog::setUserInputsEnabled(bool enable)
{
    ui->userTextBox->setEnabled(enable);
    ui->buttonBox->setEnabled(enable);
}

void OfflineLoginDialog::on_allowLongUsernames_stateChanged(int value)
{
    if (value == Qt::Checked) {
        ui->userTextBox->setMaxLength(INT_MAX);
    } else {
        ui->userTextBox->setMaxLength(16);
    }
}

// Enable the OK button only when the textbox contains something.
void OfflineLoginDialog::on_userTextBox_textEdited(const QString& newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!newText.isEmpty());
}

void OfflineLoginDialog::onTaskFailed(const QString& reason)
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

void OfflineLoginDialog::onTaskSucceeded()
{
    QDialog::accept();
}

void OfflineLoginDialog::onTaskStatus(const QString& status)
{
    ui->label->setText(status);
}

void OfflineLoginDialog::onTaskProgress(qint64 current, qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(current);
}

void OfflineLoginDialog::on_randomCharUser_clicked()
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
    ui->userTextBox->setText(randomString);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void OfflineLoginDialog::on_randomFullUser_clicked()
{
    //todo: maybe make this words configurable and not hardcoded, smh.
    QList<QString> PossibleUsernameHalves = {"Cookie", "Clicker", "Licker", "Lenny", "Super", "Sakupen", "Sonic", "Geometry", "Mining", "Chicken", "Sculpted", "Random", "Painted", "Fainted", "MadeIn", "Chinese", "Bing", "Hell", "Circles", "Wave", "Dash", "Crafting", "Smelting", "Jockey", "Vase", "Heaven", "Pudding", "Chilling"};
    int UsernameArrayLength = PossibleUsernameHalves.count();
    //int indexPossibleHalf = QRandomGenerator::global()->bounded(22);
    int indexFirstPossibleHalf = QRandomGenerator::global()->bounded(UsernameArrayLength);
    int indexSecondPossibleHalf = QRandomGenerator::global()->bounded(UsernameArrayLength);
    QString GeneratedUsername = PossibleUsernameHalves[indexFirstPossibleHalf] + PossibleUsernameHalves[indexSecondPossibleHalf];
    ui->userTextBox->setText(GeneratedUsername);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

// Public interface
BaseAccountPtr OfflineLoginDialog::newAccount(QWidget* parent, QString msg)
{
    OfflineLoginDialog dlg(parent);
    dlg.ui->label->setText(msg);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.m_account;
    }
    return nullptr;
}
