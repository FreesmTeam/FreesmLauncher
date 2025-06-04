#pragma once

#include <QDialog>
#include <QHash>

namespace Ui {
class ViewLogDialog;
}

class OtherLogsPage;

class ViewLogDialog : public QDialog {
    Q_OBJECT

   public:
    explicit ViewLogDialog(QWidget* parent = nullptr);
    ~ViewLogDialog();

   private:
    Ui::ViewLogDialog* ui;
    OtherLogsPage* m_page;
};
