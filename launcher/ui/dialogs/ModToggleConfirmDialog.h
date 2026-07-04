#pragma once

#include <QDialog>
#include <QString>

#include "minecraft/mod/Mod.h"

namespace Ui {
class ModToggleConfirmDialog;
}

/**
 * Custom MessageBox for displaying affected mods by other mods toggle.
 */
class ModToggleConfirmDialog : public QDialog {
    Q_OBJECT

   public:
    ModToggleConfirmDialog(QWidget* parent, const QSet<Mod*>& toEnable, const QSet<Mod*>& toDisable);
    ~ModToggleConfirmDialog();

    enum ModToggleConfirmCode {
        Canceled = QDialog::Rejected,
        Accepted,
        Rejected,
    };

   private slots:
    void onAcceptButtonClicked();
    void onRejectButtonClicked();
    void onCancelButtonClicked();

   private:
    Ui::ModToggleConfirmDialog* ui;
};
