
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
    QDBusMessage message = QDBusMessage::createSignal("/OfonoMessage"+mOfonoMessage->path(), "org.ofono.Message", "PropertyChanged");
    message << name << QVariant::fromValue(value);
    QDBusConnection::sessionBus().send(message);
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
