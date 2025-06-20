// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Freesm Launcher - Minecraft Launcher
 *  Copyright (C) 2025 so5iso4ka <so5iso4ka@icloud.com>
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

#include "BuildConfig.h"
#include "minecraft/auth/AuthFlow.h"
#include "minecraft/auth/AuthStep.h"
#include "net/NetJob.h"
#include "net/Upload.h"

class CustomAuthStep : public AuthStep {
    Q_OBJECT

   public:
    CustomAuthStep(AccountData* data, AuthFlow::Action action, QString password);
    virtual ~CustomAuthStep() noexcept = default;

    void perform() override;

    QString describe() override { return tr("Custom account authentication"); }

   protected:
    virtual QString authType() { return "Custom"; }

    virtual QString authUrl() { return m_data->authUrl; }

    virtual QString clientID() { return BuildConfig.LAUNCHER_NAME; }

    virtual QString requestUrl();

    QString requestTemplate();

    QString fillRequest();

    bool parseResponse();

   protected slots:
    virtual void onRequestDone();

   protected:
    std::shared_ptr<QByteArray> m_response;
    Net::Upload::Ptr m_request;
    NetJob::Ptr m_task;

    const QString m_password;
    const AuthFlow::Action m_action;
};
