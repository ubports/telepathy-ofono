
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

void NetworkRegistrationPrivate::setProperty(const QString &name, const QString& value)
{
    QDBusMessage message = QDBusMessage::createSignal(OFONO_MOCK_NETWORK_REGISTRATION_OBJECT, "org.ofono.NetworkRegistration", "PropertyChanged");
    message << name << QVariant(value);
    QDBusConnection::sessionBus().send(message);
}
