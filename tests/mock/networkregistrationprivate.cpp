
#include <QDBusConnection>

#include "networkregistrationprivateadaptor.h"

QMap<QString, NetworkRegistrationPrivate*> networkRegistrationData;

NetworkRegistrationPrivate::NetworkRegistrationPrivate(QObject *parent) :
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject(OFONO_MOCK_NETWORK_REGISTRATION_OBJECT, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    new NetworkRegistrationAdaptor(this);
}

NetworkRegistrationPrivate::~NetworkRegistrationPrivate()
{
}

QVariantMap NetworkRegistrationPrivate::GetProperties()
{
    return mProperties;
}

void NetworkRegistrationPrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}
