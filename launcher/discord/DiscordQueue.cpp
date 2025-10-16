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

#include <QDebug>
#include <utility>

#include "DiscordQueue.h"

void DiscordQueue::instanceStarted(const std::shared_ptr<BaseInstance>& instance)
{
    m_queue.enqueue(WeakRunningInstance(instance));
    emit processQueue();
}

void DiscordQueue::instanceStopped(const std::shared_ptr<BaseInstance>& instance)
{
    if (m_runningInstance.lock() == instance) {
        m_runningInstance = {};
    }

    QMutableListIterator<WeakRunningInstance> it(m_queue);
    while (it.hasNext()) {
        if (it.next().lock() == instance) {
            it.remove();
        }
    }
    emit processQueue();
}

bool DiscordQueue::isInstanceRunning()
{
    return !m_runningInstance.expired();
}

void DiscordQueue::socketReady()
{
    m_socketReady = true;
    emit processQueue();
}

void DiscordQueue::emitStarted(const RunningInstance& instance)
{
    emit started(instance);
}

void DiscordQueue::emitRest()
{
    emit rest();
}

void DiscordQueue::processQueue()
{
    if (!m_socketReady) {
        return;
    }

    if (m_runningInstance.expired() && m_queue.empty()) {
        emitRest();
        return;
    }

    if (m_runningInstance.expired()) {
        m_runningInstance = m_queue.dequeue();
        emitStarted(RunningInstance(m_runningInstance));
        return;
    }
}