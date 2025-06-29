#pragma once
#include <QWidget>
#include "BaseWizardPage.h"

namespace Ui {
class LoginWizardPage;
}

class LoginWizardPage : public BaseWizardPage {
    Q_OBJECT

   public:
    explicit LoginWizardPage(QWidget* parent = nullptr);
    ~LoginWizardPage();

    void initializePage() override;
    bool validatePage() override;
    void retranslate() override;

   private slots:
    void on_addMicrosoftButton_clicked();
    void on_addElybyButton_clicked();
    void on_addOfflineButton_clicked();
    void on_addCustomButton_clicked();

   private:
    Ui::LoginWizardPage* ui;
};
