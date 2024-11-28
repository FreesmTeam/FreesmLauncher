#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

#include <Exception.h>
#include "McClient.h"
#include <qtconcurrentrun.h>

// 7 first bits 
#define SEGMENT_BITS 0x7F
#define CONTINUE_BIT 0x80

McClient::McClient(QObject *parent, QString domain, QString ip, short port): QObject(parent), domain(domain), ip(ip), port(port) {}

QJsonObject McClient::getStatusDataBlocking() {
    qDebug() << "Connecting to socket..";
    socket.connectToHost(ip, port);

    if (!socket.waitForConnected()) {
        throw Exception("Failed to connect to socket");
    }
    qDebug() << "Connected to socket successfully";
    sendRequest();

    if (!socket.waitForReadyRead()) {
        throw Exception("Socket didn't send anything to read");
    }
    return readResponse();
}

QFuture<int> McClient::getOnlinePlayers() {
    return QtConcurrent::run([this]() {
        try {
            auto status = getStatusDataBlocking();
            int onlinePlayers = status.value("players").toObject().value("online").toInt();
            return onlinePlayers;
        } catch (const Exception &e) {
            qDebug() << "Error: " << e.what();
            return -1;
        }
    });
}

void McClient::sendRequest() {
    QByteArray data;
    writeVarInt(data, 0x00); // packet ID
    writeVarInt(data, 0x760); // protocol version
    writeVarInt(data, domain.size()); // server address length
    writeString(data, domain.toStdString()); // server address
    writeFixedInt(data, port, 2); // server port
    writeVarInt(data, 0x01); // next state
    writePacketToSocket(data); // send handshake packet

    data.clear();

    writeVarInt(data, 0x00); // packet ID
    writePacketToSocket(data); // send status packet
}

void McClient::readBytesExactFromSocket(QByteArray &resp, int bytesToRead) {
    while (bytesToRead > 0) {
        qDebug() << bytesToRead << " bytes left to read";
        if (!socket.waitForReadyRead()) {
            throw Exception("Read timeout or error");
        }

        QByteArray chunk = socket.read(bytesToRead);
        resp.append(chunk);
        bytesToRead -= chunk.size();
    }
}

QJsonObject McClient::readResponse() {
    auto resp = socket.readAll();
    int length = readVarInt(resp);

    // finish ready response
    readBytesExactFromSocket(resp, length-resp.size());

    if (length != resp.size()) {
        qDebug() << "Warning: Packet length doesn't match actual packet size (" << length << " expected vs " << resp.size() << " received)";
    }
    qDebug() << "Received response successfully";

    int packetID = readVarInt(resp);
    if (packetID != 0x00) {
        throw Exception(
            QString("Packet ID doesn't match expected value (0x00 vs 0x%1)")
            .arg(packetID, 0, 16)
        );
    }

    Q_UNUSED(readVarInt(resp)); // json length

    // 'resp' should now be the JSON string
    QJsonDocument doc = QJsonDocument::fromJson(resp);
    return doc.object();
}

// From https://wiki.vg/Protocol#VarInt_and_VarLong
void McClient::writeVarInt(QByteArray &data, int value) {
    while ((value & ~SEGMENT_BITS)) { // check if the value is too big to fit in 7 bits
        // Write 7 bits
        data.append((value & SEGMENT_BITS) | CONTINUE_BIT);

        // Erase theses 7 bits from the value to write
        // Note: >>> means that the sign bit is shifted with the rest of the number rather than being left alone
        value >>= 7;
    }
    data.append(value);
}

// From https://wiki.vg/Protocol#VarInt_and_VarLong
int McClient::readVarInt(QByteArray &data) {
    int value = 0;
    int position = 0;
    char currentByte;

    while (true) {
        currentByte = readByte(data);
        value |= (currentByte & SEGMENT_BITS) << position;

        if ((currentByte & CONTINUE_BIT) == 0) break;

        position += 7;

        if (position >= 32) throw Exception("VarInt is too big");
    }

    return value;
}

char McClient::readByte(QByteArray &data) {
    if (data.isEmpty()) {
        throw Exception("No more bytes to read");
    }

    char byte = data.at(0);
    data.remove(0, 1);
    return byte;
}

// write number with specified size in big endian format
void McClient::writeFixedInt(QByteArray &data, int value, int size) {
    for (int i = size - 1; i >= 0; i--) {
        data.append((value >> (i * 8)) & 0xFF);
    }
}

void McClient::writeString(QByteArray &data, const std::string &value) {
    data.append(value.c_str());
}

void McClient::writePacketToSocket(QByteArray &data) {
    // we prefix the packet with its length
    QByteArray dataWithSize;
    writeVarInt(dataWithSize, data.size());
    dataWithSize.append(data);

    // write it to the socket
    socket.write(dataWithSize);
    socket.flush();
}
