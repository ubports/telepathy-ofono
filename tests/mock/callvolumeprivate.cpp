
#include <QDBusConnection>

#include "callvolumeprivateadaptor.h"

QMap<QString, CallVolumePrivate*> callVolumeData;

CallVolumePrivate::CallVolumePrivate(QObject *parent) :
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject(OFONO_MOCK_CALL_VOLUME_OBJECT, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    SetProperty("Muted", QDBusVariant(QVariant(false)));

    new CallVolumeAdaptor(this);
}

CallVolumePrivate::~CallVolumePrivate()
{
}

QVariantMap CallVolumePrivate::GetProperties()
{
    return mProperties;
}

void CallVolumePrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    qDebug() << "CallVolumePrivate::SetProperty" << name << value.variant();
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}

