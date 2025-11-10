#pragma once

#include "ScrollMessageBox.h"

class UpdateCheckFailedDialog final : public ScrollMessageBox {
    Q_OBJECT

   public:
    explicit UpdateCheckFailedDialog(QWidget* parent, const QString& body);
};