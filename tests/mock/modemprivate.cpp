
#include <QDBusConnection>

#include "modemprivate.h"
#include "modemprivateadaptor.h"

ModemPrivate::ModemPrivate(OfonoModem *ofonoModem, QObject *parent) :
    QObject(parent),
    mOfonoModem(ofonoModem)
{
    QDBusConnection::sessionBus().registerObject("/ofonoQtMock", this);
    QDBusConnection::sessionBus().registerService("com.canonical.ofonoQtMock");
    new OfonoQtMockAdaptor(this);
}

ModemPrivate::~ModemPrivate()
{
}

void ModemPrivate::setOnline(bool online)
{
    mOfonoModem->setOnline(online);
}
