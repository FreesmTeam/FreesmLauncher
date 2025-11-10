#include "UpdateCheckFailedDialog.h"
#include "ui_ScrollMessageBox.h"

UpdateCheckFailedDialog::UpdateCheckFailedDialog(QWidget* parent, const QString& body)
    : ScrollMessageBox(parent,
                       tr("Failed to check for updates"),
                       tr("Could not check or get the following resources for updates:<br>"
                          "Do you wish to proceed without those resources?"),
                       body)
{
    ui->optionCheckBox->setVisible(true);
    ui->optionCheckBox->setText(tr("Disable unavailable mods"));
}