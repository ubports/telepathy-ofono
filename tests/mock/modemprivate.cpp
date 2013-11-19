
#include <QDBusConnection>

#include "modemprivate.h"
#include "modemprivateadaptor.h"
#include "ofononetworkregistration.h"

ModemPrivate::ModemPrivate(OfonoModem *ofonoModem, QObject *parent) :
    QObject(parent),
    mOfonoModem(ofonoModem)
{
    QDBusConnection::sessionBus().registerObject("/OfonoModem", this);
    QDBusConnection::sessionBus().registerService("com.canonical.OfonoQtMock");
    new OfonoModemAdaptor(this);
}

ModemPrivate::~ModemPrivate()
{
}

void ModemPrivate::setOnline(bool online)
{
    mOfonoModem->setOnline(online);
}
