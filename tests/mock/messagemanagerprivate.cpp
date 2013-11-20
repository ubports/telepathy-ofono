
#include <QDBusConnection>

#include "messagemanagerprivateadaptor.h"
#include "ofonomessagemanager.h"
#include "ofonointerface.h"

MessageManagerPrivate::MessageManagerPrivate(OfonoMessageManager *iface, OfonoInterface *prop_iface, QObject *parent) :
    mOfonoMessageManager(iface),
    mOfonoInterface(prop_iface),
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject("/OfonoMessageManager", this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    new MessageManagerAdaptor(this);
}

MessageManagerPrivate::~MessageManagerPrivate()
{
}

void MessageManagerPrivate::setProperty(const QString &name, const QString& value)
{
    mOfonoInterface->setProperty(name, value);
}

void MessageManagerPrivate::sendMessage(const QString &from, const QString &text)
{
    QVariantMap properties;
    properties["Sender"] = from;
    properties["SentTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    properties["LocalSentTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    Q_EMIT mOfonoMessageManager->incomingMessage(text, properties);
}
