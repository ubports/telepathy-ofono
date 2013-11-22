
#include <QDBusConnection>

#include "messageprivateadaptor.h"

QMap<QString, MessagePrivate*> messageData;

MessagePrivate::MessagePrivate(const QString &path, QObject *parent) :
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject("/OfonoMessage"+path, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    SetProperty("State", QDBusVariant(QVariant("pending")));

    new MessageAdaptor(this);
}

MessagePrivate::~MessagePrivate()
{
}

QVariantMap MessagePrivate::GetProperties()
{
    return mProperties;
}

void MessagePrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    qDebug() << "MessagePrivate::SetProperty" << name << value.variant();
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}

void MessagePrivate::Cancel()
{
    SetProperty("State", QDBusVariant(QVariant("failed")));
    deleteLater();
}

void MessagePrivate::MockMarkFailed()
{
    SetProperty("State", QDBusVariant(QVariant("failed")));
    deleteLater();
}

void MessagePrivate::MockMarkSent()
{
    SetProperty("State", QDBusVariant(QVariant("sent")));
    deleteLater();
}
