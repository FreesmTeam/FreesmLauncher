#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFuture>

#include <Exception.h>

// Client for the Minecraft protocol
class McClient : public QObject {
    Q_OBJECT

    QString m_domain;
    QString m_ip;
    short m_port;
    QTcpSocket m_socket;

    unsigned m_wantedRespLength = 0;
    QByteArray m_resp;

public:
    explicit McClient(QObject *parent, QString domain, QString ip, short port);
    void getStatusData();
private:
    void sendRequest();
    void readRawResponse();
    void parseResponse();

    void writeVarInt(QByteArray &data, int value);
    int readVarInt(QByteArray &data);
    char readByte(QByteArray &data);
    //! write number with specified size in big endian format
    void writeFixedInt(QByteArray &data, int value, int size);
    void writeString(QByteArray &data, const std::string &value);

    void writePacketToSocket(QByteArray &data);

    void emitFail(QString error);
    void emitSucceed(QJsonObject data);

signals:
    void succeeded(QJsonObject data);
    void failed();
    void finished();
};
