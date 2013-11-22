
#include <QDBusConnection>

#include "voicecallprivateadaptor.h"
#include "ofonointerface.h"
#include "ofonovoicecall.h"

VoiceCallPrivate::VoiceCallPrivate(OfonoVoiceCall *iface, OfonoInterface *prop_iface, const QVariantMap &initialProperties, QObject *parent) :
    mOfonoVoiceCall(iface),
    mOfonoInterface(prop_iface),
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject("/OfonoVoiceCall"+iface->path(), this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    SetProperty("LineIdentification", QDBusVariant(QVariant("")));
    SetProperty("IncomingLine", QDBusVariant(QVariant("")));
    SetProperty("Name", QDBusVariant(QVariant("")));
    SetProperty("Multiparty", QDBusVariant(QVariant(false)));
    SetProperty("State", QDBusVariant(QVariant("")));
    SetProperty("StartTime", QDBusVariant(QVariant("")));
    SetProperty("Information", QDBusVariant(QVariant("")));
    SetProperty("Icon", QDBusVariant(QVariant("")));
    SetProperty("Emergency", QDBusVariant(QVariant(false)));
    SetProperty("RemoteHeld", QDBusVariant(QVariant(false)));
    SetProperty("RemoteMultiparty", QDBusVariant(QVariant(false)));

    qDebug() << initialCallProperties;
    QMapIterator<QString, QVariant> i(initialCallProperties);
    while (i.hasNext()) {
        i.next();
        SetProperty(i.key(), QDBusVariant(i.value()));
    }

    new VoiceCallAdaptor(this);
}

VoiceCallPrivate::~VoiceCallPrivate()
{
}

QVariantMap VoiceCallPrivate::GetProperties()
{
    return mProperties;
}

void VoiceCallPrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    qDebug() << "VoiceCallPrivate::SetProperty" << name << value.variant();
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}

void VoiceCallPrivate::Hangup()
{
    SetProperty("State", QDBusVariant(QVariant("disconnected")));
    deleteLater();
}

void VoiceCallPrivate::Answer()
{
    if (mProperties["State"] == "active") {
        SetProperty("State", QDBusVariant(QVariant("active")));
    }
}

