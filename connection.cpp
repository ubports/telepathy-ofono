/**
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Tiago Salem Herrmann <tiago.herrmann@canonical.com>
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

#include "mmsdmessage.h"
#include "mmsdservice.h"
// audioflinger
#ifdef USE_AUDIOFLINGER
#include <waudio.h>
#endif

#ifdef USE_PULSEAUDIO
#include "qpulseaudioengine.h"
#endif

static void enable_earpiece()
{
#ifdef USE_AUDIOFLINGER
    char parameter[20];
    int i;
    /* Set the call mode in AudioFlinger */
    AudioSystem_setMode(AUDIO_MODE_IN_CALL);
    sprintf(parameter, "routing=%d", AUDIO_DEVICE_OUT_EARPIECE);
    /* Try the first 3 threads, as this is not fixed and there's no easy
     * way to retrieve the default thread/output from Android */
    for (i = 1; i <= 3; i++) {
        if (AudioSystem_setParameters(i, parameter) >= 0)
            break;
    }
#endif
#ifdef USE_PULSEAUDIO
    QPulseAudioEngine::instance()->setCallMode(true, false);
#endif
}

static void enable_normal()
{
#ifdef USE_AUDIOFLINGER
    char parameter[20];
    int i;
    /* Set normal mode in AudioFlinger */
    AudioSystem_setMode(AUDIO_MODE_NORMAL);
    /* Get device back to speaker mode, as by default in_call
     * mode sets up device out to earpiece */
    sprintf(parameter, "routing=%d", AUDIO_DEVICE_OUT_SPEAKER);
    /* Try the first 3 threads, as this is not fixed and there's no easy
     * way to retrieve the default thread/output from Android */
    for (i = 1; i <= 3; i++) {
        if (AudioSystem_setParameters(i, parameter) >= 0)
            break;
    }
#endif
#ifdef USE_PULSEAUDIO
    QPulseAudioEngine::instance()->setCallMode(false, false);
#endif
}

static void enable_speaker()
{
#ifdef USE_AUDIOFLINGER
    char parameter[20];
    int i;
    /* Set the call mode in AudioFlinger */
    AudioSystem_setMode(AUDIO_MODE_IN_CALL);
    sprintf(parameter, "routing=%d", AUDIO_DEVICE_OUT_SPEAKER);
    /* Try the first 3 threads, as this is not fixed and there's no easy
     * way to retrieve the default thread/output from Android */
    for (i = 1; i <= 3; i++) {
        if (AudioSystem_setParameters(i, parameter) >= 0)
            break;
    }
#endif
#ifdef USE_PULSEAUDIO
    QPulseAudioEngine::instance()->setCallMode(true, true);
#endif
}

static void enable_ringtone()
{
#ifdef USE_AUDIOFLINGER
    char parameter[20];
    int i;
    /* Set the call mode in AudioFlinger */
    AudioSystem_setMode(AUDIO_MODE_IN_CALL);
    sprintf(parameter, "routing=%d", AUDIO_DEVICE_OUT_SPEAKER);
    /* Try the first 3 threads, as this is not fixed and there's no easy
     * way to retrieve the default thread/output from Android */
    for (i = 1; i <= 3; i++) {
        if (AudioSystem_setParameters(i, parameter) >= 0)
            break;
    }
#endif
#ifdef USE_PULSEAUDIO
    QPulseAudioEngine::instance()->setCallMode(false, true);
#endif
}

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
    mRegisterTimer(new QTimer(this)),
    mMmsdManager(new MMSDManager(this)),
    mSpeakerMode(false)
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

    QObject::connect(mMmsdManager, SIGNAL(serviceAdded(const QString&)), SLOT(onMMSDServiceAdded(const QString&)));
    QObject::connect(mMmsdManager, SIGNAL(serviceRemoved(const QString&)), SLOT(onMMSDServiceRemoved(const QString&)));

    // update audio route
    QObject::connect(mOfonoVoiceCallManager, SIGNAL(callAdded(QString,QVariantMap)), SLOT(updateAudioRoute()));
    QObject::connect(mOfonoVoiceCallManager, SIGNAL(callRemoved(QString)), SLOT(updateAudioRoute()));

    // workaround: we can't add services here as tp-ofono interfaces are not exposed on dbus
    // todo: use QDBusServiceWatcher
    QTimer::singleShot(1000, this, SLOT(onCheckMMSServices()));
}

void oFonoConnection::onCheckMMSServices()
{
    Q_FOREACH(QString servicePath, mMmsdManager->services()) {
        onMMSDServiceAdded(servicePath);
    }
}

void oFonoConnection::onMMSDServiceAdded(const QString &path)
{
    qDebug() << "oFonoConnection::onMMSServiceAdded" << path;
    MMSDService *service = new MMSDService(path, this);
    mMmsdServices[path] = service;
    QObject::connect(service, SIGNAL(messageAdded(const QString&, const QVariantMap&)), SLOT(onMMSAdded(const QString&, const QVariantMap&)));
    QObject::connect(service, SIGNAL(messageRemoved(const QString&)), SLOT(onMMSRemoved(const QString&)));
    Q_FOREACH(MessageStruct message, service->messages()) {
        addMMSToService(message.path.path(), message.properties, service->path());
    }
}

void oFonoConnection::onMMSPropertyChanged(QString property, QVariant value)
{
    qDebug() << "oFonoConnection::onMMSPropertyChanged" << property << value;
    if (property == "Status") {
        if (value == "sent") {
            // FIXME send delivery report
        }
    }
}

void oFonoConnection::onMMSDServiceRemoved(const QString &path)
{
    MMSDService *service = mMmsdServices.take(path);
    if (!service) {
        qWarning() << "oFonoConnection::onMMSServiceRemoved failed" << path;
        return;
    }

    // remove all messages from this service
    Q_FOREACH(MMSDMessage *message, mServiceMMSList[service->path()]) {
        qDebug() << "removing message " <<  message->path() << " from service " << service->path();
        message->deleteLater();
        mServiceMMSList[service->path()].removeAll(message);
    }
    mServiceMMSList.remove(service->path());
    service->deleteLater();
    qDebug() << "oFonoConnection::onMMSServiceRemoved" << path;
}

void oFonoConnection::addMMSToService(const QString &path, const QVariantMap &properties, const QString &servicePath)
{
    qDebug() << "addMMSToService " << path << properties << servicePath;
    MMSDMessage *msg = new MMSDMessage(path, properties);
    QObject::connect(msg, SIGNAL(propertyChanged(QString,QVariant)), SLOT(onMMSPropertyChanged(QString,QVariant)));
    mServiceMMSList[servicePath].append(msg);
    if (properties["Status"] ==  "received") {
        const QString normalizedNumber = PhoneNumberUtils::normalizePhoneNumber(properties["Sender"].toString());
        if (!PhoneNumberUtils::isPhoneNumber(normalizedNumber)) {
            qCritical() << "Error creating channel for incoming message: phone number not valid: " << properties["Sender"].toString();
            return;
        }

        // check if there is an open channel for this number and use it
        Q_FOREACH(const QString &phoneNumber, mTextChannels.keys()) {
            if (PhoneNumberUtils::compareLoosely(normalizedNumber, phoneNumber)) {
                qDebug() << "existing channel" << mTextChannels[phoneNumber];
                mTextChannels[phoneNumber]->mmsReceived(path, properties);
                return;
            }
        }

        Tp::DBusError error;
        bool yours;
        qDebug() << "new handle" << normalizedNumber;
        uint handle = newHandle(normalizedNumber);
        ensureChannel(TP_QT_IFACE_CHANNEL_TYPE_TEXT,Tp::HandleTypeContact, handle, yours, handle, false, &error);
        if(error.isValid()) {
            qCritical() << "Error creating channel for incoming message " << error.name() << error.message();
            return;
        }
        mTextChannels[normalizedNumber]->mmsReceived(path, properties);
    }
}

void oFonoConnection::onMMSAdded(const QString &path, const QVariantMap &properties)
{
    qDebug() << "oFonoConnection::onMMSAdded" << path << properties;
    MMSDService *service = qobject_cast<MMSDService*>(sender());
    if (!service) {
        qWarning() << "oFonoConnection::onMMSAdded failed";
        return;
    }

    addMMSToService(path, properties, service->path());
}

void oFonoConnection::onMMSRemoved(const QString &path)
{
    qDebug() << "oFonoConnection::onMMSRemoved" << path;
    MMSDService *service = qobject_cast<MMSDService*>(sender());
    if (!service) {
        qWarning() << "oFonoConnection::onMMSRemoved failed";
        return;
    }

    // remove this message from the service
    Q_FOREACH(MMSDMessage *message, mServiceMMSList[service->path()]) {
        if (message->path() == path) {
            message->deleteLater();
            mServiceMMSList[service->path()].removeAll(message);
            break;
        }
    }
}

oFonoConnection::~oFonoConnection() {
    mOfonoModemManager->deleteLater();
    mOfonoMessageManager->deleteLater();
    mOfonoVoiceCallManager->deleteLater();
    mOfonoCallVolume->deleteLater();
    mOfonoNetworkRegistration->deleteLater();
    mRegisterTimer->deleteLater();
    Q_FOREACH(MMSDService *service, mMmsdServices) {
        onMMSDServiceRemoved(service->path());
    }
   
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
    QObject::connect(mTextChannels[newPhoneNumber], SIGNAL(messageRead(QString)), SLOT(onMessageRead(QString)));
    QObject::connect(mTextChannels[newPhoneNumber], SIGNAL(destroyed()), SLOT(onTextChannelClosed()));
    qDebug() << mTextChannels[newPhoneNumber];
    return mTextChannels[newPhoneNumber]->baseChannel();
}

void oFonoConnection::onMessageRead(const QString &id)
{
    Q_FOREACH(QList<MMSDMessage*> messages, mServiceMMSList.values()) {
        Q_FOREACH(MMSDMessage* message, messages) {
            if (message->path() == id) {
                message->markRead();
                message->remove();
                return;
            }
        }
    }
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
        qWarning() << "Error creating channel for incoming message";
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
        qWarning() << "Error creating channel for incoming message" << error.name() << error.message();
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
        qWarning() << "Error creating handle for " << phoneNumber;
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
        qWarning() << "Error creating channel for incoming call - not a valid number: " << properties["LineIdentification"].toString();
        return;
    }

    // check if there is an open channel for this number, if so, ignore it
    Q_FOREACH(const QString &phoneNumber, mCallChannels.keys()) {
        if (PhoneNumberUtils::compareLoosely(normalizedNumber, phoneNumber)) {
            qWarning() << "call channel for this number already exists: " << phoneNumber;
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
        qWarning() << "error creating the channel " << error.name() << error.message();
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

bool oFonoConnection::speakerMode()
{
    return mSpeakerMode;
}

void oFonoConnection::setSpeakerMode(bool active)
{
    if (mSpeakerMode != active) {
        mSpeakerMode = active;
        updateAudioRoute();
        Q_EMIT speakerModeChanged(active);
    }
}

void oFonoConnection::updateAudioRoute()
{
    int currentCalls = mOfonoVoiceCallManager->getCalls().size();
    if (currentCalls != 0) {
        if (currentCalls == 1) {
            // if we have only one call, check if it's incoming and
            // enable speaker mode so the ringtone is audible
            OfonoVoiceCall *call = new OfonoVoiceCall(mOfonoVoiceCallManager->getCalls().first());
            if (call) {
                if (call->state() == "incoming") {
                    enable_ringtone();
                    call->deleteLater();
                    return;
                }
                call->deleteLater();
            }
        }
        if(mSpeakerMode) {
            enable_speaker();
        } else {
            enable_earpiece();
        }
    } else {
        enable_normal();
        setSpeakerMode(false);
    }

}


