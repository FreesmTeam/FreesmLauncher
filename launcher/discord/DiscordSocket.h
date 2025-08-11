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

#include <QLocalSocket>
#include <QObject>

class DiscordSocket : public QObject {
    enum class Opcode : uint32_t { Handshake = 0, Frame = 1, Close = 2, Ping = 3, Pong = 4 };

    Q_OBJECT
   public:
    DiscordSocket();

    ~DiscordSocket() override = default;

    virtual void connectSocket() = 0;

    bool send(const QByteArray& data, Opcode opcode = Opcode::Frame);

    QString errorString() const { return m_socket.errorString(); }

    bool isConnected() const { return m_socket.state() == QLocalSocket::ConnectedState; }

   signals:
    void connected();
    void failed();

   protected:
    void handshake();

   protected slots:
    void onConnected();
    virtual void errorOccurred(QLocalSocket::LocalSocketError socketError);

   protected:
    QLocalSocket m_socket;

    static constexpr auto s_opcodeSize = 4;
    static constexpr auto s_lengthSize = 4;
};

class UnixDiscordSocket : public DiscordSocket {
    Q_OBJECT

   public:
    void connectSocket() override;

   protected slots:
    void errorOccurred(QLocalSocket::LocalSocketError socketError) override;

   private slots:
    void tryNext();

   private:
    int m_socketIndex{};
};

class WinDiscordSocket : public DiscordSocket {
    Q_OBJECT

   public:
    void connectSocket() override;

   protected slots:
    void errorOccurred(QLocalSocket::LocalSocketError socketError) override;

   private slots:
    void tryNext();

   private:
    int m_socketIndex{};
};

std::unique_ptr<DiscordSocket> makeSocket();