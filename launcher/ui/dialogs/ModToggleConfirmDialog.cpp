#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "ModToggleConfirmDialog.h"
#include "ui_ModToggleConfirmDialog.h"
#include "minecraft/mod/ModFolderModel.h"
// ToDo: remove these dirty hacks and change translation files
#include "ui/dialogs/CopyInstanceDialog.h"

/**
 * Dirty hack to use ModFolderModel context for translation
 */
static inline QString translate(const char *s, const char *c = nullptr, int n = -1)
{
    return ModFolderModel::staticMetaObject.tr(s, c, n).trimmed();
}

/**
 * Even more dirty hack, because ModFolderModel doesn't have translation for "Cancel" button
 */
static inline QString translate_cancel(const char *s, const char *c = nullptr, int n = -1)
{
    return CopyInstanceDialog::staticMetaObject.tr(s, c, n);
}

static QString formatMod(Mod* mod)
{
    if (mod->version().isEmpty()) {
        return QString("- %1").arg(mod->name());
    } else {
        return QString("- %1 (%2)").arg(mod->name(), mod->version());
    }
}

ModToggleConfirmDialog::ModToggleConfirmDialog(QWidget* parent, const QSet<Mod*>& toEnable, const QSet<Mod*>& toDisable)
    : QDialog(parent), ui(new Ui::ModToggleConfirmDialog)
{
    ui->setupUi(this);
    resize(minimumSizeHint());

    if (toEnable.size() > 0 && toDisable.size() > 0) {
        setWindowTitle(translate("Confirm toggle"));

        ui->introLabel->setText(translate("Toggling these mod(s) will cause changes to other mods.\n"));
        ui->toEnableLabel->setText(translate("%n mod(s) will be enabled\n", "", toEnable.size()));
        ui->toDisableLabel->setText(translate("%n mod(s) will be disabled\n", "", toDisable.size()));
        ui->questionLabel->setText(translate("Do you want to automatically apply these related changes?\nIgnoring them may break the game."));

        ui->confirmButton->setText(translate("Toggle Required Mods"));
        ui->rejectButton->setText(translate("Only Toggle Selected"));
    } else if (toEnable.size() > 0) {
        setWindowTitle(translate("Confirm enable"));

        ui->toEnableLabel->setText(translate("The enabled mod(s) require %n mod(s).\n", "", toEnable.size()));
        ui->questionLabel->setText(translate("Would you like to enable them as well?\nIgnoring them may break the game."));

        ui->confirmButton->setText(translate("Enable Required"));
        ui->rejectButton->setText(translate("Only Enable Selected"));
    } else {
        setWindowTitle(translate("Confirm disable"));

        ui->toDisableLabel->setText(translate("The disabled mod(s) are required by %n mod(s).\n", "", toDisable.size()));
        ui->questionLabel->setText(translate("Would you like to disable them as well?\nIgnoring them may break the game."));

        ui->confirmButton->setText(translate("Disable Required"));
        ui->rejectButton->setText(translate("Only Disable Selected"));
    }

    ui->cancelButton->setText(translate_cancel("Cancel"));

    ui->confirmButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    ui->rejectButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    ui->cancelButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));

    if (toEnable.isEmpty() || toDisable.isEmpty())
    {
        ui->introLabel->setVisible(false);
    }

    if (toEnable.isEmpty())
    {
        ui->toEnableLabel->setVisible(false);
        ui->toEnableList->setVisible(false);
    }

    for (auto mod : toEnable)
    {
        ui->toEnableList->addItem(formatMod(mod));
    }

    if (toDisable.isEmpty())
    {
        ui->toDisableLabel->setVisible(false);
        ui->toDisableList->setVisible(false);
    }

    for (auto mod : toDisable)
    {
        ui->toDisableList->addItem(formatMod(mod));
    }

    connect(ui->confirmButton, &QPushButton::clicked, this, &ModToggleConfirmDialog::onAcceptButtonClicked);
    connect(ui->rejectButton, &QPushButton::clicked, this, &ModToggleConfirmDialog::onRejectButtonClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &ModToggleConfirmDialog::onCancelButtonClicked);
}

ModToggleConfirmDialog::~ModToggleConfirmDialog()
{
    delete ui;
}

void ModToggleConfirmDialog::onAcceptButtonClicked()
{
    done(Accepted);
}

void ModToggleConfirmDialog::onRejectButtonClicked()
{
    done(Rejected);
}

void ModToggleConfirmDialog::onCancelButtonClicked()
{
    done(Canceled);
}
