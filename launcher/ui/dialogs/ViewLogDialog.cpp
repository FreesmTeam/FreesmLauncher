#include "ViewLogDialog.h"
#include "ui_ViewLogDialog.h"

#include "ui/pages/instance/OtherLogsPage.h"

ViewLogDialog::ViewLogDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ViewLogDialog)
    , m_page(new OtherLogsPage("launcher-logs", tr("Launcher Logs"), "Launcher-Logs", nullptr, parent))
{
    ui->setupUi(this);
    ui->verticalLayout->insertWidget(0, m_page);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_page->opened();
}

ViewLogDialog::~ViewLogDialog()
{
    m_page->closed();
    delete ui;
}
