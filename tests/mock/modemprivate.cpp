
#include <QDBusConnection>

#include "modemprivate.h"
#include "modemprivateadaptor.h"
#include "ofononetworkregistration.h"

ModemPrivate::ModemPrivate(OfonoModem *ofonoModem, QObject *parent) :
    QObject(parent),
    mOfonoModem(ofonoModem)
{
    QDBusConnection::sessionBus().registerObject(OFONO_MOCK_MODEM_OBJECT, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    new ModemAdaptor(this);
}

ModemPrivate::~ModemPrivate()
{
}

void ModemPrivate::setOnline(bool online)
{
    setProperty("Powered", online);
    setProperty("Online", online);
}

void ModemPrivate::setProperty(const QString &name, const QVariant& value)
{
    QDBusMessage message = QDBusMessage::createSignal(OFONO_MOCK_MODEM_OBJECT, "org.ofono.Modem", "PropertyChanged");
    message << name << QVariant::fromValue(QDBusVariant(value));
    QDBusConnection::sessionBus().send(message);
}

