#include <QCoreApplication>

#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/Debug>

#include "protocol.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    Tp::BaseProtocolPtr proto = Tp::BaseProtocol::create<Protocol>(
            QDBusConnection::sessionBus(), QLatin1String("ofono"));
    Tp::BaseConnectionManagerPtr cm = Tp::BaseConnectionManager::create(
            QDBusConnection::sessionBus(), QLatin1String("ofono"));
    cm->addProtocol(proto);
    cm->registerObject();

    return a.exec();
}
