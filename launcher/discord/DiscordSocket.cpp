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

#include <QDataStream>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>

#include "BuildConfig.h"

#include "DiscordSocket.h"

namespace {
QByteArray generateHandshake()
{
    QJsonObject request;

    request["v"] = 1;
    request["client_id"] = BuildConfig.DISCORD_CLIENT_ID;

    return QJsonDocument(request).toJson();
}
}  // namespace

DiscordSocket::DiscordSocket()
{
    connect(&m_socket, &QLocalSocket::connected, this, &DiscordSocket::onConnected);
    connect(&m_socket, &QLocalSocket::errorOccurred, this, &DiscordSocket::errorOccurred);
}

DiscordSocket::~DiscordSocket()
{
    m_socket.abort();
}

void DiscordSocket::onConnected()
{
    connect(&m_socket, &QLocalSocket::readyRead, this, &DiscordSocket::read);

    handshake();
}

void DiscordSocket::handshake()
{
    enqueue(generateHandshake(), Opcode::Handshake);
}

void DiscordSocket::processReply()
{
    const auto reply = QJsonDocument::fromJson(m_msg);
    const auto obj = reply.object();
    if (obj["cmd"] == "DISPATCH" && obj["evt"] == "READY") {
        emit connected();
        return;
    }

    if (!m_messagesToSend.isEmpty()) {
        emit send();
    }
}

void DiscordSocket::send()
{
    m_state = State::Sent;
    auto [data, opcode] = m_messagesToSend.dequeue();

    QByteArray frame;
    QDataStream stream(&frame, QIODevice::WriteOnly);

    stream.setByteOrder(QDataStream::LittleEndian);

    stream << static_cast<quint32>(opcode);
    stream << static_cast<quint32>(data.size());

    frame.append(data);

    const auto sent = m_socket.write(frame);
    if (sent != frame.size()) {
        qDebug() << "send failed, sent " << sent << '/' << frame.size();
        return;
    }
    m_socket.flush();
}

void DiscordSocket::errorOccurred([[maybe_unused]] QLocalSocket::LocalSocketError socketError)
{
    emit failed();
}

void DiscordSocket::read()
{
    if (m_state != State::Reading) {
        if (m_socket.bytesAvailable() >= sizeof(quint32) * 2) {
            QDataStream in(&m_socket);
            in.setByteOrder(QDataStream::LittleEndian);

            quint32 op;
            in >> op;

            quint32 bytes;
            in >> bytes;
            m_pendingBytes = bytes;

            m_state = State::Reading;
            m_msg = {};
        } else {
            return;
        }
    }

    if (m_state == State::Reading) {
        auto toRead = std::min(m_socket.bytesAvailable(), m_pendingBytes);
        m_msg.append(m_socket.read(toRead));
        m_pendingBytes -= toRead;
        if (m_pendingBytes == 0) {
            m_state = State::Waiting;
            processReply();
        }
    }
}

void DiscordSocket::enqueue(const QByteArray& data, Opcode opcode)
{
    m_messagesToSend.enqueue({ data, opcode });
    if (m_state == State::Waiting) {
        emit send();
    }
}

void UnixDiscordSocket::connectSocket()
{
    const QString runtimeDir = QProcessEnvironment::systemEnvironment().value("XDG_RUNTIME_DIR");
    m_path = runtimeDir != "" ? runtimeDir : QDir::tempPath();
    emit tryNext();
}

void UnixDiscordSocket::errorOccurred(QLocalSocket::LocalSocketError socketError)
{
    if (socketError == QLocalSocket::ServerNotFoundError) {
        m_socket.abort();
        m_socketIndex++;
        emit tryNext();
    } else {
        emit failed();
    }
}

void UnixDiscordSocket::tryNext()
{
    if (m_socketIndex < 10) {
        const QString path = m_path + "/discord-ipc-" + QString::number(m_socketIndex);
        m_socket.setServerName(path);
        m_socket.connectToServer();
    } else {
        emit failed();
    }
}

void WinDiscordSocket::connectSocket()
{
    emit tryNext();
}

void WinDiscordSocket::errorOccurred(QLocalSocket::LocalSocketError socketError)
{
    if (socketError == QLocalSocket::ServerNotFoundError) {
        m_socket.abort();
        m_socketIndex++;
        emit tryNext();
    } else {
        emit failed();
    }
}

void WinDiscordSocket::tryNext()
{
    if (m_socketIndex < 10) {
        const QString path = "discord-ipc-" + QString::number(m_socketIndex);
        m_socket.setServerName(path);
        m_socket.connectToServer();
    } else {
        emit failed();
    }
}

std::unique_ptr<DiscordSocket> makeSocket()
{
#ifdef Q_OS_WINDOWS
    auto socket = std::make_unique<WinDiscordSocket>();
#else
    auto socket = std::make_unique<UnixDiscordSocket>();
#endif
    return socket;
}