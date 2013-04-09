#include "protocol.h"
#include "connection.h"

#include <TelepathyQt/RequestableChannelClassSpec>
#include <TelepathyQt/RequestableChannelClassSpecList>

Protocol::Protocol(const QDBusConnection &dbusConnection, const QString &name)
    : Tp::BaseProtocol(dbusConnection, name)
{
    setRequestableChannelClasses(Tp::RequestableChannelClassSpecList() <<
                                 Tp::RequestableChannelClassSpec::textChat() <<
                                 Tp::RequestableChannelClassSpec::audioCall());

    setCreateConnectionCallback(memFun(this, &Protocol::createConnection));
}

Tp::BaseConnectionPtr Protocol::createConnection(const QVariantMap &parameters, Tp::DBusError *error) {
    Q_UNUSED(error);
    return Tp::BaseConnection::create<oFonoConnection>(QDBusConnection::sessionBus(), "ofono", name().toLatin1(), parameters);
}
