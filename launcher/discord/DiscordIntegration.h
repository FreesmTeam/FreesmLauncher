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

#include <QObject>
#include "RunningInstance.h"

class BaseInstance;
class DiscordSocket;
class DiscordQueue;

class DiscordIntegration : public QObject {
    Q_OBJECT

   public:
    explicit DiscordIntegration(bool showAlways);
    ~DiscordIntegration() override;

   public:
    void instanceStarted(const std::shared_ptr<BaseInstance>& instance);
    void instanceStopped(const std::shared_ptr<BaseInstance>& instance);

    void showAlways(bool state);

   private slots:
    void socketConnected();
    void socketFailed();

    void startActivity(RunningInstance instance);
    void stopActivity();

   private:
    std::unique_ptr<DiscordSocket> m_socket;

    std::unique_ptr<DiscordQueue> m_queue;

    bool m_showAlways;
    qint64 m_startTime;
};
