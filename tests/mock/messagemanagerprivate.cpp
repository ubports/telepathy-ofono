
#include <QDBusConnection>

#include "messagemanagerprivateadaptor.h"
#include "ofonomessagemanager.h"
#include "ofonomessage.h"
#include "ofonointerface.h"

MessageManagerPrivate::MessageManagerPrivate(OfonoMessageManager *iface, OfonoInterface *prop_iface, QObject *parent) :
    mOfonoMessageManager(iface),
    mOfonoInterface(prop_iface),
    messageCount(0),
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject(OFONO_MOCK_MESSAGE_MANAGER_OBJECT, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    SetProperty("ServiceCenterAddress", QDBusVariant(QVariant("")));
    SetProperty("UseDeliveryReports", QDBusVariant(QVariant(false)));
    SetProperty("Bearer", QDBusVariant(QVariant("")));
    SetProperty("Alphabet", QDBusVariant(QVariant("")));
    SetProperty("UseDeliveryReports", QDBusVariant(QVariant(false)));
    SetProperty("UseDeliveryReports", QDBusVariant(QVariant(false)));
    new MessageManagerAdaptor(this);
}

MessageManagerPrivate::~MessageManagerPrivate()
{
}

QVariantMap MessageManagerPrivate::GetProperties()
{
    return mProperties;
}

void MessageManagerPrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    qDebug() << "MessageManagerPrivate::SetProperty" << name << value.variant();
    mProperties[name] = value.variant();
    QDBusMessage message = QDBusMessage::createSignal(OFONO_MOCK_MESSAGE_MANAGER_OBJECT, "org.ofono.MessageManager", "PropertyChanged");
    message << name << QVariant::fromValue(value);
    QDBusConnection::sessionBus().send(message);
}

void MessageManagerPrivate::MockSendMessage(const QString &from, const QString &text)
{
    QVariantMap properties;
    properties["Sender"] = from;
    properties["SentTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    properties["LocalSentTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QDBusMessage message = QDBusMessage::createSignal(OFONO_MOCK_MESSAGE_MANAGER_OBJECT, "org.ofono.MessageManager", "IncomingMessage");
    message << text << properties;
    QDBusConnection::sessionBus().send(message);
}


QDBusObjectPath MessageManagerPrivate::SendMessage(const QString &to, const QString &text)
{
    QString newPath("/OfonoMessage"+QString::number(++messageCount));
    mMessages[newPath] = new OfonoMessage(newPath);
    connect(mMessages[newPath], SIGNAL(destroyed()), this, SLOT(onMessageDestroyed()));

    QDBusMessage message = QDBusMessage::createSignal(OFONO_MOCK_MESSAGE_MANAGER_OBJECT, "org.ofono.MessageManager", "MessageAdded");
    message << newPath  ;
    QDBusConnection::sessionBus().send(message);


    return QDBusObjectPath(newPath);
}

void MessageManagerPrivate::onMessageDestroyed()
{
    OfonoMessage *message = static_cast<OfonoMessage*>(sender());
    if (message) {
        QDBusMessage message = QDBusMessage::createSignal(OFONO_MOCK_MESSAGE_MANAGER_OBJECT, "org.ofono.MessageManager", "MessageRemoved");
        message << message.path();
        QDBusConnection::sessionBus().send(message);

    }
}
