/**
 * Copyright (C) 2012-2013 Canonical, Ltd.
 *
 * Authors:
 *  Tiago Salem Herrmann <tiago.herrmann@canonical.com>
 *
 * This file is part of telepathy-ofono.
 *
 * telepathy-ofono is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * telepathy-ofono is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
    mOfonoCallVolume(new OfonoCallVolume(OfonoModem::AutomaticSelect,"")),
    mHandleCount(0)
{
    setSelfHandle(newHandle("<SelfHandle>"));

    setConnectCallback(Tp::memFun(this,&oFonoConnection::connect));
    setInspectHandlesCallback(Tp::memFun(this,&oFonoConnection::inspectHandles));
    setRequestHandlesCallback(Tp::memFun(this,&oFonoConnection::requestHandles));
    setCreateChannelCallback(Tp::memFun(this,&oFonoConnection::createChannel));

    // initialise requests interface (Connection.Interface.Requests)
    requestsIface = Tp::BaseConnectionRequestsInterface::create(this);

    // set requestable text channel properties
    Tp::RequestableChannelClass text;
    text.fixedProperties[TP_QT_IFACE_CHANNEL+".ChannelType"] = TP_QT_IFACE_CHANNEL_TYPE_TEXT;
    text.fixedProperties[TP_QT_IFACE_CHANNEL+".TargetHandleType"]  = Tp::HandleTypeContact;
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL+".TargetHandle");
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL+".TargetID");

    // set requestable call channel properties
    Tp::RequestableChannelClass call;
    call.fixedProperties[TP_QT_IFACE_CHANNEL+".ChannelType"] = TP_QT_IFACE_CHANNEL_TYPE_CALL;
    call.fixedProperties[TP_QT_IFACE_CHANNEL+".TargetHandleType"]  = Tp::HandleTypeContact;
    call.fixedProperties[TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialAudio"]  = true;
    call.allowedProperties.append(TP_QT_IFACE_CHANNEL+".TargetHandle");
    call.allowedProperties.append(TP_QT_IFACE_CHANNEL+".TargetID");
    call.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialAudio");
    call.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialVideo");
    call.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialAudioName");
    call.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialVideoName");
    call.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialTransport");
    call.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL+".HardwareStreaming");

    requestsIface->requestableChannelClasses << text << call;

    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(requestsIface));

    QObject::connect(mOfonoMessageManager, SIGNAL(incomingMessage(QString,QVariantMap)), this, SLOT(onOfonoIncomingMessage(QString,QVariantMap)));
    QObject::connect(mOfonoVoiceCallManager, SIGNAL(callAdded(QString,QVariantMap)), SLOT(onOfonoCallAdded(QString, QVariantMap)));
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
    qDebug() << "oFonoConnection::inspectHandles " << identifiers;
    return identifiers;
}

void oFonoConnection::connect(Tp::DBusError *error) {
    qDebug() << "oFonoConnection::connect";
    setStatus(Tp::ConnectionStatusConnected, Tp::ConnectionStatusReasonRequested);
}

Tp::UIntList oFonoConnection::requestHandles(uint handleType, const QStringList& identifiers, Tp::DBusError* error)
{
    qDebug() << "requestHandles";
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
    qDebug() << "requestHandles" << handles;
    return handles;
}

Tp::BaseChannelPtr oFonoConnection::createTextChannel(uint targetHandleType,
                                               uint targetHandle, Tp::DBusError *error)
{
    Q_UNUSED(targetHandleType);
    Q_UNUSED(error);

    QString newPhoneNumber = mHandles.value(targetHandle);

    Q_FOREACH(const QString &phoneNumber, mTextChannels.keys()) {
        if (PhoneNumberUtils::compareLoosely(phoneNumber, newPhoneNumber)) {
            return mTextChannels[phoneNumber]->baseChannel();
        }
    }

    mTextChannels[newPhoneNumber] = new oFonoTextChannel(this, newPhoneNumber, targetHandle);
    QObject::connect(mTextChannels[newPhoneNumber], SIGNAL(destroyed()), SLOT(onTextChannelClosed()));
    qDebug() << mTextChannels[newPhoneNumber];
    return mTextChannels[newPhoneNumber]->baseChannel();
}

Tp::BaseChannelPtr oFonoConnection::createCallChannel(uint targetHandleType,
                                               uint targetHandle, Tp::DBusError *error)
{
    Q_UNUSED(targetHandleType);

    QString newPhoneNumber = mHandles.value(targetHandle);

    Q_FOREACH(const QString &phoneNumber, mCallChannels.keys()) {
        if (PhoneNumberUtils::compareLoosely(phoneNumber, newPhoneNumber)) {
            return mCallChannels[phoneNumber]->baseChannel();
        }
    }

    bool isOngoingCall = false;
    QDBusObjectPath objpath;
    Q_FOREACH(const QString &callId, mOfonoVoiceCallManager->getCalls()) {
        // check if this is an ongoing call
        OfonoVoiceCall *call = new OfonoVoiceCall(callId);
        if (PhoneNumberUtils::compareLoosely(call->lineIdentification(), newPhoneNumber)) {
            isOngoingCall = true;
        }
        call->deleteLater();
        if (isOngoingCall) {
            objpath.setPath(callId);
            break;
        }
    }

    if (!isOngoingCall) {
        objpath = mOfonoVoiceCallManager->dial(newPhoneNumber, "");
    }

    if (objpath.path().isEmpty()) {
        error->set(TP_QT_ERROR_NOT_AVAILABLE, "Channel could not be created");
        return Tp::BaseChannelPtr();
    }

    mCallChannels[newPhoneNumber] = new oFonoCallChannel(this, newPhoneNumber, targetHandle,objpath.path());
    QObject::connect(mCallChannels[newPhoneNumber], SIGNAL(destroyed()), SLOT(onCallChannelClosed()));
    qDebug() << mCallChannels[newPhoneNumber];
    return mCallChannels[newPhoneNumber]->baseChannel();

}

Tp::BaseChannelPtr oFonoConnection::createChannel(const QString& channelType, uint targetHandleType,
                                               uint targetHandle, Tp::DBusError *error)
{
    qDebug() << "oFonoConnection::createChannel" << targetHandle;
    if( (targetHandleType != Tp::HandleTypeContact) || targetHandle == 0 || !mHandles.keys().contains(targetHandle)) {
        error->set(TP_QT_ERROR_INVALID_HANDLE, "Handle not found");
        return Tp::BaseChannelPtr();
    }

    if (channelType == TP_QT_IFACE_CHANNEL_TYPE_TEXT) {
        return createTextChannel(targetHandleType, targetHandle, error);
    } else if (channelType == TP_QT_IFACE_CHANNEL_TYPE_CALL) {
        return createCallChannel(targetHandleType, targetHandle, error);
    } else {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, "Channel type not available");
    }

    return Tp::BaseChannelPtr();
}

OfonoMessageManager *oFonoConnection::messageManager()
{
    return mOfonoMessageManager;
}

OfonoVoiceCallManager *oFonoConnection::voiceCallManager()
{
    return mOfonoVoiceCallManager;
}

OfonoCallVolume *oFonoConnection::callVolume()
{
    return mOfonoCallVolume;
}

void oFonoConnection::onOfonoIncomingMessage(const QString &message, const QVariantMap &info)
{
    const QString normalizedNumber = PhoneNumberUtils::normalizePhoneNumber(info["Sender"].toString());
    if (!PhoneNumberUtils::isPhoneNumber(normalizedNumber)) {
        qDebug() << "Error creating channel for incoming message";
        return;
    }

    // check if there is an open channel for this number and use it
    Q_FOREACH(const QString &phoneNumber, mTextChannels.keys()) {
        if (PhoneNumberUtils::compareLoosely(normalizedNumber, phoneNumber)) {
            mTextChannels[phoneNumber]->messageReceived(message, info);
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

void oFonoConnection::onCallChannelClosed()
{
    qDebug() << "onCallChannelClosed()";
    oFonoCallChannel *channel = static_cast<oFonoCallChannel*>(sender());
    if (channel) {
        QString key = mCallChannels.key(channel);
        qDebug() << "call channel closed for number " << key;
        mCallChannels.remove(key);
    }
}

uint oFonoConnection::ensureHandle(const QString &phoneNumber)
{
    QString normalizedNumber = PhoneNumberUtils::normalizePhoneNumber(phoneNumber);
    if (!PhoneNumberUtils::isPhoneNumber(normalizedNumber)) {
        qDebug() << "Error creating handle for " << phoneNumber;
        return 0;
    }

    Q_FOREACH(const QString &phone, mHandles.values()) {
        if (PhoneNumberUtils::compareLoosely(normalizedNumber, phone)) {
            // this user already exists
            return mHandles.key(phone);
        }
    }
    return newHandle(normalizedNumber);
}

void oFonoConnection::onOfonoCallAdded(const QString &call, const QVariantMap &properties)
{
    qDebug() << "new call" << call << properties;

    bool yours;
    Tp::DBusError error;
    const QString normalizedNumber = PhoneNumberUtils::normalizePhoneNumber(properties["LineIdentification"].toString());
    if (!PhoneNumberUtils::isPhoneNumber(normalizedNumber)) {
        qDebug() << "Error creating channel for incoming call";
        return;
    }

    // check if there is an open channel for this number, if so, ignore it
    Q_FOREACH(const QString &phoneNumber, mCallChannels.keys()) {
        if (PhoneNumberUtils::compareLoosely(normalizedNumber, phoneNumber)) {
            qDebug() << "call channel for this number already exists";
            return;
        }
    }

    uint handle = ensureHandle(normalizedNumber);
    uint initiatorHandle = 0;
    if (properties["State"] == "incoming" || properties["State"] == "waiting") {
        initiatorHandle = handle;
    } else {
        initiatorHandle = selfHandle();
    }

    qDebug() << "initiatorHandle " <<initiatorHandle;
    qDebug() << "handle" << handle;

    Tp::BaseChannelPtr channel  = ensureChannel(TP_QT_IFACE_CHANNEL_TYPE_CALL, Tp::HandleTypeContact, handle, yours, initiatorHandle, false, &error);
    if (error.isValid() || channel.isNull()) {
        qDebug() << "error creating the channel";
        return;
    }
}
