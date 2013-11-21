
#include <QDBusConnection>

#include "networkregistrationprivateadaptor.h"
#include "ofonointerface.h"
#include "ofononetworkregistration.h"

NetworkRegistrationPrivate::NetworkRegistrationPrivate(OfonoNetworkRegistration *iface, OfonoInterface *prop_iface, QObject *parent) :
    mOfonoNetworkRegistration(iface),
    mOfonoInterface(prop_iface),
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
