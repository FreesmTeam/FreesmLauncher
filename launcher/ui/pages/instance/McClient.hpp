#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

#define SEGMENT_BITS 0x7F
#define CONTINUE_BIT 0x80

// Client for the Minecraft protocol
class McClient : public QObject {
    Q_OBJECT

    QString domain;
    QString ip;
    short port;
    QTcpSocket socket;

public:
    explicit McClient(QObject *parent, QString domain, QString ip, short port): QObject(parent), domain(domain), ip(ip), port(port) {}

    QJsonObject getStatusData() {
        qDebug() << "Connecting to socket..";
        socket.connectToHost(ip, port);

        if (!socket.waitForConnected(3000)) {
            throw std::runtime_error("Failed to connect to socket");
        }
        qDebug() << "Connected to socket successfully";
        sendRequest();

        if (!socket.waitForReadyRead(3000)) {
            throw std::runtime_error("Socket didn't send anything to read");
        }
        return readResponse();
    }

    int getOnlinePlayers() {
        auto status = getStatusData();
        return status.value("players").toObject().value("online").toInt();
    }

    void sendRequest() {
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

    void readBytesExactFromSocket(QByteArray &resp, int bytesToRead) {
        while (bytesToRead > 0) {
            qDebug() << bytesToRead << " bytes left to read";
            if (!socket.waitForReadyRead()) {
                throw std::runtime_error("Read timeout or error");
            }

            QByteArray chunk = socket.read(qMin(bytesToRead, socket.bytesAvailable()));
            resp.append(chunk);
            bytesToRead -= chunk.size();
        }
    }

    QJsonObject readResponse() {
        auto resp = socket.readAll();
        int length = readVarInt(resp);

        // finish ready response
        readBytesExactFromSocket(resp, length-resp.size());

        if (length != resp.size()) {
            printf("Warning: Packet length doesn't match actual packet size (%d expected vs %d received)\n", length, resp.size());
        }
        qDebug() << "Received response successfully";

        int packetID = readVarInt(resp);
        if (packetID != 0x00) {
            throw std::runtime_error(
                QString("Packet ID doesn't match expected value (0x00 vs 0x%1)")
                .arg(packetID, 0, 16).toStdString()
            );
        }

        int jsonLength = readVarInt(resp);
        std::string json = resp.toStdString();

        QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
        return doc.object();
    }

    void close() {
        socket.close();
    }

private:
    // From https://wiki.vg/Protocol#VarInt_and_VarLong
    void writeVarInt(QByteArray &data, int value) {
        while (true) {
            if ((value & ~SEGMENT_BITS) == 0) {
                data.append(value);
                return;
            }

            data.append((value & SEGMENT_BITS) | CONTINUE_BIT);

            // Note: >>> means that the sign bit is shifted with the rest of the number rather than being left alone
            value >>= 7;
        }
    }

    // From https://wiki.vg/Protocol#VarInt_and_VarLong
    int readVarInt(QByteArray &data) {
        int value = 0;
        int position = 0;
        char currentByte;

        while (true) {
            currentByte = readByte(data);
            value |= (currentByte & SEGMENT_BITS) << position;

            if ((currentByte & CONTINUE_BIT) == 0) break;

            position += 7;

            if (position >= 32) throw std::runtime_error("VarInt is too big");
        }

        return value;
    }

    char readByte(QByteArray &data) {
        if (data.isEmpty()) {
            throw std::runtime_error("No more bytes to read");
        }

        char byte = data.at(0);
        data.remove(0, 1);
        return byte;
    }

    // write number with specified size in big endian format
    void writeFixedInt(QByteArray &data, int value, int size) {
        for (int i = size - 1; i >= 0; i--) {
            data.append((value >> (i * 8)) & 0xFF);
        }
    }

    void writeString(QByteArray &data, std::string value) {
        data.append(value);
    }

    void writePacketToSocket(QByteArray &data) {
        // we prefix the packet with its length
        QByteArray dataWithSize;
        writeVarInt(dataWithSize, data.size());
        dataWithSize.append(data);

        // write it to the socket
        socket.write(dataWithSize);
        socket.flush();
    }
};
