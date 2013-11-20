
#include <QDBusConnection>

#include "messageprivateadaptor.h"
#include "ofonointerface.h"
#include "ofonomessage.h"

MessagePrivate::MessagePrivate(OfonoMessage *iface, OfonoInterface *prop_iface, QObject *parent) :
    mOfonoMessage(iface),
    mOfonoInterface(prop_iface),
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject("/OfonoMessage"+iface->path(), this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    new MessageAdaptor(this);
}

MessagePrivate::~MessagePrivate()
{
}

void MessagePrivate::setProperty(const QString &name, const QString& value)
{
    mOfonoInterface->setProperty(name, value);
}

