#include <QObject>
#include <QString>
#include <QDnsLookup>
#include <QtNetwork/qtcpsocket.h>
#include <QHostInfo>

// resolve the IP and port of a Minecraft server
class McResolver : public QObject {
    Q_OBJECT

    QString constrDomain;
    int constrPort;

public:
    explicit McResolver(QObject *parent, QString domain, int port);
    void ping();

private:
    void pingWithDomainSRV(QString domain, int port);
    void pingWithDomainA(QString domain, int port);
    void emitFail(std::string error);
    void emitSucceed(QString ip, int port);

signals:
    void succeeded(QString ip, int port);
    void failed();
    void finished();
};
