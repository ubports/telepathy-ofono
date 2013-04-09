#include <QDebug>

#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>

// ofono-qt
#include <ofonomodem.h>

// telepathy-ofono
#include "connection.h"
#include "phoneutils.h"
#include "protocol.h"

oFonoConnection::oFonoConnection(const QDBusConnection &dbusConnection,
                            const QString &cmName,
                            const QString &protocolName,
                            const QVariantMap &parameters) :
    Tp::BaseConnection(dbusConnection, cmName, protocolName, parameters),
    mOfonoModemManager(new OfonoModemManager(this)),
    mOfonoMessageManager(new OfonoMessageManager(OfonoModem::AutomaticSelect,"")),
    mOfonoVoiceCallManager(new OfonoVoiceCallManager(OfonoModem::AutomaticSelect,"")),
    mHandleCount(0)
{
    setSelfHandle(newHandle("<SelfHandle>"));

    setConnectCallback(Tp::memFun(this,&oFonoConnection::connect));
    setInspectHandlesCallback(Tp::memFun(this,&oFonoConnection::inspectHandles));
    setRequestHandlesCallback(Tp::memFun(this,&oFonoConnection::requestHandles));
    setCreateChannelCallback(Tp::memFun(this,&oFonoConnection::createChannel));

    requestsIface = Tp::BaseConnectionRequestsInterface::create(this);
    Tp::RequestableChannelClass text;
    text.fixedProperties[TP_QT_IFACE_CHANNEL+".ChannelType"] = TP_QT_IFACE_CHANNEL_TYPE_TEXT;
    text.fixedProperties[TP_QT_IFACE_CHANNEL+".TargetHandleType"]  = Tp::HandleTypeContact;
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL+".TargetHandle");
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL+".TargetID");

    Tp::RequestableChannelClass call;
    call.fixedProperties[TP_QT_IFACE_CHANNEL+".ChannelType"] = TP_QT_IFACE_CHANNEL_TYPE_CALL;
    call.fixedProperties[TP_QT_IFACE_CHANNEL+".TargetHandleType"]  = Tp::HandleTypeContact;
    call.allowedProperties.append(TP_QT_IFACE_CHANNEL+".TargetHandle");
    call.allowedProperties.append(TP_QT_IFACE_CHANNEL+".TargetID");

    requestsIface->requestableChannelClasses << text << call;

    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(requestsIface));

    QObject::connect(mOfonoMessageManager, SIGNAL(incomingMessage(QString,QVariantMap)), this, SLOT(oFonoIncomingMessage(QString,QVariantMap)));
    QObject::connect(mOfonoVoiceCallManager, SIGNAL(callAdded(QString)), SLOT(oFonoCallAdded(QString)));
}

uint oFonoConnection::newHandle(const QString &identifier)
{
    mHandles[++mHandleCount] = identifier;
    return mHandleCount;
}

oFonoConnection::~oFonoConnection() {
}

QStringList oFonoConnection::inspectHandles(uint handleType, const Tp::UIntList& handles, Tp::DBusError *error)
{
    QStringList identifiers;

    if( handleType != Tp::HandleTypeContact ) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT,"Not supported");
        return QStringList();
    }

    qDebug() << "oFonoConnection::inspectHandles " << handles;
    Q_FOREACH( uint handle, handles) {
        if (mHandles.keys().contains(handle)) {
            identifiers.append(mHandles.value(handle));
        } else {
            error->set(TP_QT_ERROR_INVALID_HANDLE, "Handle not found");
            return QStringList();
        }
    }

    return identifiers;
}

void oFonoConnection::connect(Tp::DBusError *error) {
    qDebug() << "oFonoConnection::connect";
    setStatus(Tp::ConnectionStatusConnected, Tp::ConnectionStatusReasonRequested);
}

Tp::UIntList oFonoConnection::requestHandles(uint handleType, const QStringList& identifiers, Tp::DBusError* error)
{
    Tp::UIntList handles;

    if( handleType != Tp::HandleTypeContact ) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, "Not supported");
        return Tp::UIntList();
    }

    Q_FOREACH( const QString& identifier, identifiers) {
        const QString normalizedNumber = PhoneNumberUtils::normalizePhoneNumber(identifier);
        if (mHandles.values().contains(normalizedNumber)) {
            handles.append(mHandles.key(normalizedNumber));
        } else if (PhoneNumberUtils::isPhoneNumber(normalizedNumber)) {
            handles.append(newHandle(normalizedNumber));
        } else {
            error->set(TP_QT_ERROR_INVALID_HANDLE, "Handle not found");
            return Tp::UIntList();
        }
    }

    return handles;
}

Tp::BaseChannelPtr oFonoConnection::createChannel(const QString& channelType, uint targetHandleType,
                                               uint targetHandle, Tp::DBusError *error)
{
    if( (targetHandleType != Tp::HandleTypeContact) || targetHandle == 0 || !mHandles.keys().contains(targetHandle)) {
        error->set(TP_QT_ERROR_INVALID_HANDLE, "Handle not found");
        return Tp::BaseChannelPtr();
    }

    if( channelType != TP_QT_IFACE_CHANNEL_TYPE_TEXT && channelType != TP_QT_IFACE_CHANNEL_TYPE_CALL) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, "Channel type not available");
    }

    QString newPhoneNumber = mHandles.value(targetHandle);
    if (channelType == TP_QT_IFACE_CHANNEL_TYPE_TEXT) {
        Q_FOREACH(const QString &phoneNumber, mTextChannels.keys()) {
            if (PhoneNumberUtils::compareLoosely(phoneNumber, newPhoneNumber)) {
                return mTextChannels[phoneNumber]->getBaseChannel();
            }
        }
    } else if (channelType == TP_QT_IFACE_CHANNEL_TYPE_CALL) {
        Q_FOREACH(const QString &phoneNumber, mCallChannels.keys()) {
            if (PhoneNumberUtils::compareLoosely(phoneNumber, newPhoneNumber)) {
                return mCallChannels[phoneNumber];
            }
        }
    }

    if (channelType == TP_QT_IFACE_CHANNEL_TYPE_TEXT) {
        mTextChannels[newPhoneNumber] = new oFonoTextChannel(this, newPhoneNumber, targetHandle);
        QObject::connect(mTextChannels[newPhoneNumber], SIGNAL(destroyed()), SLOT(onTextChannelClosed()));
        qDebug() << mTextChannels[newPhoneNumber];
        return mTextChannels[newPhoneNumber]->getBaseChannel();
    }
    return Tp::BaseChannelPtr();
}

OfonoMessageManager *oFonoConnection::getMessageManager()
{
    return mOfonoMessageManager;
}

OfonoVoiceCallManager *oFonoConnection::getVoiceCallManager()
{
    return mOfonoVoiceCallManager;
}

void oFonoConnection::oFonoIncomingMessage(const QString &message, const QVariantMap &info)
{
    const QString normalizedNumber = PhoneNumberUtils::normalizePhoneNumber(info["Sender"].toString());
    if (!PhoneNumberUtils::isPhoneNumber(normalizedNumber)) {
        qDebug() << "Error creating channel for incoming message";
        return;
    }

    // check if there is an open channel for this number and use it
    if (mHandles.values().contains(normalizedNumber)) {
        if (mTextChannels.keys().contains(normalizedNumber)) {
            mTextChannels[normalizedNumber]->messageReceived(message, info);
            return;
        }
    }

    Tp::DBusError error;
    bool yours;
    uint handle = newHandle(normalizedNumber);
    ensureChannel(TP_QT_IFACE_CHANNEL_TYPE_TEXT,Tp::HandleTypeContact, handle, yours, handle, false, &error);
    if(error.isValid()) {
        qDebug() << "Error creating channel for incoming message";
        return;
    }
    mTextChannels[normalizedNumber]->messageReceived(message, info);
}

void oFonoConnection::onTextChannelClosed()
{
    oFonoTextChannel *channel = static_cast<oFonoTextChannel*>(sender());
    if (channel) {
        QString key = mTextChannels.key(channel);
        qDebug() << "text channel closed for number " << key;
        mTextChannels.remove(key);
    }
}

void oFonoConnection::oFonoCallAdded(const QString &call)
{
    // TODO: implement
}
