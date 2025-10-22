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
#include <QQueue>

#include "RunningInstance.h"

class BaseInstance;

class DiscordQueue : public QObject {
    Q_OBJECT

   public:
    void instanceStarted(const std::shared_ptr<BaseInstance>& instance);
    void instanceStopped(const std::shared_ptr<BaseInstance>& instance);

    bool isInstanceRunning();

   public slots:
    void socketReady();

   signals:
    void started(const RunningInstance instance);
    void rest();

   private:
    void emitStarted(const RunningInstance& instance);
    void emitRest();

   private slots:
    void processQueue();

   private:
    WeakRunningInstance m_runningInstance;

    QQueue<WeakRunningInstance> m_queue;

    bool m_socketReady{};
};
