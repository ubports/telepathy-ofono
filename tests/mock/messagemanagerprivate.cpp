
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
    Q_EMIT PropertyChanged(name, value);
}

void MessageManagerPrivate::MockSendMessage(const QString &from, const QString &text)
{
    QVariantMap properties;
    properties["Sender"] = from;
    properties["SentTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    properties["LocalSentTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    Q_EMIT IncomingMessage(text, properties);
}


QDBusObjectPath MessageManagerPrivate::SendMessage(const QString &to, const QString &text)
{
    QString newPath("/OfonoMessage"+QString::number(++messageCount));
    QDBusObjectPath newPathObj(newPath);
    mMessages[newPath] = new OfonoMessage(newPath);
    connect(mMessages[newPath], SIGNAL(destroyed()), this, SLOT(onMessageDestroyed()));

    Q_EMIT MessageAdded(newPathObj, QVariantMap());

    return newPathObj;
}

void MessageManagerPrivate::onMessageDestroyed()
{
    OfonoMessage *message = static_cast<OfonoMessage*>(sender());
    if (message) {
        mMessages.remove(message->path());
        Q_EMIT MessageRemoved(QDBusObjectPath(message->path()));
    }
}
