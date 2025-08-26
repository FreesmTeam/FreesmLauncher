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

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

#include "Activity.h"
#include "Application.h"
#include "BaseInstance.h"
#include "DiscordQueue.h"
#include "DiscordSocket.h"

#include "DiscordIntegration.h"

DiscordIntegration::DiscordIntegration() : m_socket(makeSocket()), m_queue(std::make_unique<DiscordQueue>())
{
    connect(m_socket.get(), &DiscordSocket::connected, this, &DiscordIntegration::socketConnected);
    connect(m_socket.get(), &DiscordSocket::failed, this, &DiscordIntegration::socketFailed);

    connect(m_queue.get(), &DiscordQueue::started, this, &DiscordIntegration::startActivity);
    connect(m_queue.get(), &DiscordQueue::rest, this, &DiscordIntegration::stopActivity);

    m_socket->connectSocket();
}

DiscordIntegration::~DiscordIntegration() = default;

void DiscordIntegration::instanceStarted(const std::shared_ptr<BaseInstance>& instance)
{
    m_queue->instanceStarted(instance);
}

void DiscordIntegration::instanceStopped(const std::shared_ptr<BaseInstance>& instance)
{
    m_queue->instanceStopped(instance);
}

bool DiscordIntegration::startActivity(const RunningInstance instance)
{
    return m_socket->send(generateActivity(instance));
}

bool DiscordIntegration::stopActivity()
{
    return m_socket->send(generateEmptyActivity());
}

void DiscordIntegration::socketConnected()
{
    qDebug() << "Connected to Discord";
    emit m_queue->socketReady();
}

void DiscordIntegration::socketFailed()
{
    qDebug() << "Couldn't connect to Discord: " << m_socket->errorString();
}