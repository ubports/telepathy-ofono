#ifndef OFONOPROTOCOL_H
#define OFONOPROTOCOL_H

#include <TelepathyQt/BaseProtocol>

class Protocol : public Tp::BaseProtocol
{
    Q_OBJECT
    Q_DISABLE_COPY(Protocol)

public:
    Protocol(const QDBusConnection &dbusConnection, const QString &name);

private:
    Tp::BaseConnectionPtr createConnection(const QVariantMap &parameters, Tp::DBusError *error);
};

#endif
