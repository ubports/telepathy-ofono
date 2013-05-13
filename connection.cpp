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
#include <TelepathyQt/DBusObject>

// ofono-qt
#include <ofonomodem.h>

// telepathy-ofono
#include "connection.h"
#include "phoneutils.h"
#include "protocol.h"

// miliseconds
#define OFONO_REGISTER_RETRY_TIME 5000

oFonoConnection::oFonoConnection(const QDBusConnection &dbusConnection,
                            const QString &cmName,
                            const QString &protocolName,
                            const QVariantMap &parameters) :
    Tp::BaseConnection(dbusConnection, cmName, protocolName, parameters),
    mOfonoModemManager(new OfonoModemManager(this)),
    mOfonoMessageManager(new OfonoMessageManager(OfonoModem::AutomaticSelect,"")),
    mOfonoVoiceCallManager(new OfonoVoiceCallManager(OfonoModem::AutomaticSelect,"")),
    mOfonoCallVolume(new OfonoCallVolume(OfonoModem::AutomaticSelect,"")),
    mOfonoNetworkRegistration(new OfonoNetworkRegistration(OfonoModem::AutomaticSelect, "")),
    mOfonoMessageWaiting(new OfonoMessageWaiting(OfonoModem::AutomaticSelect, "")),
    mHandleCount(0),
    mRegisterTimer(new QTimer(this))
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

    // init presence interface
    simplePresenceIface = Tp::BaseConnectionSimplePresenceInterface::create();
    simplePresenceIface->setSetPresenceCallback(Tp::memFun(this,&oFonoConnection::setPresence));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(simplePresenceIface));

    // init custom voicemail interface (not provided by telepathy)
    voicemailIface = BaseConnectionVoicemailInterface::create();
    voicemailIface->setVoicemailCountCallback(Tp::memFun(this,&oFonoConnection::voicemailCount));
    voicemailIface->setVoicemailIndicatorCallback(Tp::memFun(this,&oFonoConnection::voicemailIndicator));
    voicemailIface->setVoicemailNumberCallback(Tp::memFun(this,&oFonoConnection::voicemailNumber));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(voicemailIface));

    // Set Presence
    Tp::SimpleStatusSpec presenceOnline;
    presenceOnline.type = Tp::ConnectionPresenceTypeAvailable;
    presenceOnline.maySetOnSelf = true;
    presenceOnline.canHaveMessage = false;

    Tp::SimpleStatusSpec presenceOffline;
    presenceOffline.type = Tp::ConnectionPresenceTypeOffline;
    presenceOffline.maySetOnSelf = false;
    presenceOffline.canHaveMessage = false;

    Tp::SimpleStatusSpecMap statuses;
    statuses.insert(QLatin1String("available"), presenceOnline);
    statuses.insert(QLatin1String("offline"), presenceOffline);

    simplePresenceIface->setStatuses(statuses);
    mSelfPresence.type = Tp::ConnectionPresenceTypeOffline;
    mRequestedSelfPresence.type = Tp::ConnectionPresenceTypeOffline;

    bool validModem = false;
    if (mOfonoVoiceCallManager->modem()) {
        validModem = mOfonoVoiceCallManager->modem()->isValid();
        if (validModem) {
            QObject::connect(mOfonoVoiceCallManager->modem(), SIGNAL(onlineChanged(bool)), SLOT(onValidityChanged(bool)));
        }
    }
    // force update current presence
    onOfonoNetworkRegistrationChanged(mOfonoNetworkRegistration->status());

    contactsIface = Tp::BaseConnectionContactsInterface::create();
    contactsIface->setGetContactAttributesCallback(Tp::memFun(this,&oFonoConnection::getContactAttributes));
    contactsIface->setContactAttributeInterfaces(QStringList()
                                                 << TP_QT_IFACE_CONNECTION
                                                 << TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE);
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(contactsIface));

    QObject::connect(mOfonoMessageManager, SIGNAL(incomingMessage(QString,QVariantMap)), this, SLOT(onOfonoIncomingMessage(QString,QVariantMap)));
    QObject::connect(mOfonoVoiceCallManager, SIGNAL(callAdded(QString,QVariantMap)), SLOT(onOfonoCallAdded(QString, QVariantMap)));
    QObject::connect(mOfonoVoiceCallManager, SIGNAL(validityChanged(bool)), SLOT(onValidityChanged(bool)));
    QObject::connect(mOfonoNetworkRegistration, SIGNAL(statusChanged(QString)), SLOT(onOfonoNetworkRegistrationChanged(QString)));
    QObject::connect(mOfonoMessageWaiting, SIGNAL(voicemailMessageCountChanged(int)), voicemailIface.data(), SLOT(setVoicemailCount(int)));
    QObject::connect(mOfonoMessageWaiting, SIGNAL(voicemailWaitingChanged(bool)), voicemailIface.data(), SLOT(setVoicemailIndicator(bool)));

    QObject::connect(mRegisterTimer, SIGNAL(timeout()), SLOT(onTryRegister()));
}

oFonoConnection::~oFonoConnection() {
    mOfonoModemManager->deleteLater();
    mOfonoMessageManager->deleteLater();
    mOfonoVoiceCallManager->deleteLater();
    mOfonoCallVolume->deleteLater();
    mOfonoNetworkRegistration->deleteLater();
    mRegisterTimer->deleteLater();
}

void oFonoConnection::onTryRegister()
{
    qDebug() << "onTryRegister";
    bool networkRegistered = isNetworkRegistered();
    if (networkRegistered) {
        setOnline(networkRegistered);
        mRegisterTimer->stop();
        return;
    }

    // if we have modem, check if it is online
    OfonoModem *modem = mOfonoNetworkRegistration->modem();
    if (modem) {
        if (!modem->online()) {
            modem->setOnline(true);
        }
        mOfonoNetworkRegistration->registerOp();
    }
}

bool oFonoConnection::isNetworkRegistered()
{
    QString status = mOfonoNetworkRegistration->status();
    return  !(!mOfonoNetworkRegistration->modem() ||
              !mOfonoNetworkRegistration->modem()->online() ||
              status == "unregistered" ||
              status == "denied" ||
              status == "unknown" ||
              status == "searching" ||
              status.isEmpty());
}

void oFonoConnection::onOfonoNetworkRegistrationChanged(const QString &status)
{
    qDebug() << "onOfonoNetworkRegistrationChanged" << status << "is network registered: " << isNetworkRegistered();
    if (!isNetworkRegistered() && mRequestedSelfPresence.type == Tp::ConnectionPresenceTypeAvailable) {
        setOnline(false);
        onTryRegister();
        mRegisterTimer->setInterval(OFONO_REGISTER_RETRY_TIME);
        mRegisterTimer->start();
        return;
    }
    setOnline(isNetworkRegistered());
}

uint oFonoConnection::setPresence(const QString& status, const QString& statusMessage, Tp::DBusError *error)
{
    qDebug() << "setPresence" << status;
    if (status == "available") {
        mRequestedSelfPresence.type = Tp::ConnectionPresenceTypeAvailable;
        if (!isNetworkRegistered()) {
            onTryRegister();
            mRegisterTimer->setInterval(OFONO_REGISTER_RETRY_TIME);
            mRegisterTimer->start();
        }
    }
    return selfHandle();
}

Tp::ContactAttributesMap oFonoConnection::getContactAttributes(const Tp::UIntList &handles, const QStringList &ifaces, Tp::DBusError *error)
{
    qDebug() << "getContactAttributes" << handles << ifaces;
    Tp::ContactAttributesMap attributesMap;
    QVariantMap attributes;
    Q_FOREACH(uint handle, handles) {
        attributes[TP_QT_IFACE_CONNECTION+"/contact-id"] = inspectHandles(Tp::HandleTypeContact, Tp::UIntList() << handle, error).at(0);
        if (ifaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE)) {
            attributes[TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE+"/presence"] = QVariant::fromValue(mSelfPresence);
        }
        attributesMap[handle] = attributes;
    }
    return attributesMap;
}

void oFonoConnection::onValidityChanged(bool valid)
{
    qDebug() << "validityChanged" << valid << "is network registered: " << isNetworkRegistered() << mRequestedSelfPresence.type;
    QObject::disconnect(mOfonoVoiceCallManager->modem(), 0,0,0);
    QObject::connect(mOfonoVoiceCallManager->modem(), SIGNAL(onlineChanged(bool)), SLOT(onValidityChanged(bool)));
    if (!isNetworkRegistered() && mRequestedSelfPresence.type == Tp::ConnectionPresenceTypeAvailable) {
        setOnline(false);
        onTryRegister();
        mRegisterTimer->setInterval(OFONO_REGISTER_RETRY_TIME);
        mRegisterTimer->start();
    }
}

void oFonoConnection::setOnline(bool online)
{
    qDebug() << "setOnline" << online;
    Tp::SimpleContactPresences presences;
    if (online) {
        mSelfPresence.status = "available";
        mSelfPresence.statusMessage = "";
        mSelfPresence.type = Tp::ConnectionPresenceTypeAvailable;
    } else {
        mSelfPresence.status = "offline";
        mSelfPresence.statusMessage = "";
        mSelfPresence.type = Tp::ConnectionPresenceTypeOffline;
    }
    presences[selfHandle()] = mSelfPresence;
    simplePresenceIface->setPresences(presences);
}

uint oFonoConnection::newHandle(const QString &identifier)
{
    mHandles[++mHandleCount] = identifier;
    return mHandleCount;
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

    bool success = true;
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
        objpath = mOfonoVoiceCallManager->dial(newPhoneNumber, "", success);
    }
    qDebug() << "success " << success;
    if (objpath.path().isEmpty() || !success) {
        if (!success) {
            error->set(TP_QT_ERROR_NOT_AVAILABLE, mOfonoVoiceCallManager->errorMessage());
        } else {
            error->set(TP_QT_ERROR_NOT_AVAILABLE, "Channel could not be created");
        }
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

    if (mSelfPresence.type != Tp::ConnectionPresenceTypeAvailable) {
        error->set(TP_QT_ERROR_NETWORK_ERROR, "No network available");
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

uint oFonoConnection::voicemailCount(Tp::DBusError *error)
{
    return mOfonoMessageWaiting->voicemailMessageCount();
}

QString oFonoConnection::voicemailNumber(Tp::DBusError *error)
{
    return mOfonoMessageWaiting->voicemailMailboxNumber();
}

bool oFonoConnection::voicemailIndicator(Tp::DBusError *error)
{
    return mOfonoMessageWaiting->voicemailWaiting();
}
