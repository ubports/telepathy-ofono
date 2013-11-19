
#include <QDBusConnection>

#include "networkregistrationprivateadaptor.h"
#include "ofonointerface.h"

NetworkRegistrationPrivate::NetworkRegistrationPrivate(OfonoNetworkRegistration *iface, OfonoInterface *prop_iface, QObject *parent) :
    mOfonoNetworkRegistration(iface),
    mOfonoInterface(prop_iface),
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject("/OfonoNetworkRegistration", this);
    QDBusConnection::sessionBus().registerService("com.canonical.OfonoQtMock");
    new NetworkRegistrationAdaptor(this);
}

NetworkRegistrationPrivate::~NetworkRegistrationPrivate()
{
}

void NetworkRegistrationPrivate::setProperty(const QString &name, const QString& value)
{
    mOfonoInterface->setProperty(name, value);
}
