
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
    SetProperty("Powered", QDBusVariant(QVariant(false)));
    SetProperty("Online", QDBusVariant(QVariant(false)));
    SetProperty("Lockdown", QDBusVariant(QVariant(false)));
    SetProperty("Emergency", QDBusVariant(QVariant(false)));
    SetProperty("Name", QDBusVariant(QVariant("Mock Modem")));
    SetProperty("Manufacturer", QDBusVariant(QVariant("Canonical")));
    SetProperty("Model", QDBusVariant(QVariant("Mock Modem")));
    SetProperty("Revision", QDBusVariant(QVariant("1.0")));
    SetProperty("Serial", QDBusVariant(QVariant("ABC123")));
    SetProperty("Type", QDBusVariant(QVariant("Software")));
    SetProperty("Features", QDBusVariant(QVariant(QStringList() << "gprs" << "cbs" << "net" << "sms" << "sim")));
    SetProperty("Interfaces", QDBusVariant(QVariant(QStringList() << "org.ofono.ConnectionManager" << "org.ofono.AssistedSatelliteNavigation" << "org.ofono.CellBroadcast" << "org.ofono.NetworkRegistration" << "org.ofono.CallVolume" << "org.ofono.CallMeter" << "org.ofono.SupplementaryServices" << "org.ofono.CallBarring" << "org.ofono.CallSettings" << "org.ofono.MessageWaiting" << "org.ofono.SmartMessaging" << "org.ofono.PushNotification" << "org.ofono.MessageManager" << "org.ofono.Phonebook" << "org.ofono.TextTelephony" << "org.ofono.CallForwarding" << "org.ofono.SimToolkit" << "org.ofono.NetworkTime" << "org.ofono.VoiceCallManager" << "org.ofono.SimManager")));

    new ModemAdaptor(this);
}

ModemPrivate::~ModemPrivate()
{
}

void ModemPrivate::MockSetOnline(bool online)
{
    SetProperty("Powered", QDBusVariant(QVariant(online)));
    SetProperty("Online", QDBusVariant(QVariant(online)));
}

QVariantMap ModemPrivate::GetProperties()
{
    return mProperties;
}

void ModemPrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}

