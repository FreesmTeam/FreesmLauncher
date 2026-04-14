// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Freesm Launcher - Minecraft Launcher
 *  Copyright (C) 2026 so5iso4ka <so5iso4ka@icloud.com>
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

#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <memory>

#include "minecraft/auth/AuthStep.h"
#include "net/NetJob.h"
#include "net/Upload.h"

class CustomRefreshStep : public AuthStep {
    Q_OBJECT

   public:
    explicit CustomRefreshStep(AccountData* data);
    ~CustomRefreshStep() override;

    void perform() override;

    QString describe() override;

   private:
    QJsonObject fillRequest() const;

   private slots:
    virtual void onRequestDone();

   private:
    std::shared_ptr<QByteArray> m_response;
    Net::Upload::Ptr m_request;
    NetJob::Ptr m_task;
};