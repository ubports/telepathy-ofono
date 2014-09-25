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
 *          Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
 */

#include <QDebug>

#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/DBusObject>

// telepathy-ofono
#include "connection.h"
#include "phoneutils_p.h"
#include "protocol.h"
#include "ofonoconferencecallchannel.h"

#include "mmsdmessage.h"
#include "mmsdservice.h"

#ifdef USE_PULSEAUDIO
#include "qpulseaudioengine.h"
#endif

#include "sqlitedatabase.h"
#include "pendingmessagesmanager.h"
#include "dbustypes.h"

static void enable_earpiece()
{
#ifdef USE_PULSEAUDIO
    QPulseAudioEngine::instance()->setCallMode(QPulseAudioEngine::CallActive, AudioModeBtOrWiredOrEarpiece);
#endif
}

static void enable_normal()
{
#ifdef USE_PULSEAUDIO
    QPulseAudioEngine::instance()->setCallMode(QPulseAudioEngine::CallEnded, AudioModeWiredOrSpeaker);
    QPulseAudioEngine::instance()->setMicMute(false);
#endif
}

static void enable_speaker()
{
#ifdef USE_PULSEAUDIO
    QPulseAudioEngine::instance()->setCallMode(QPulseAudioEngine::CallActive, AudioModeSpeaker);
#endif
}

static void enable_ringtone()
{
#ifdef USE_PULSEAUDIO
    QPulseAudioEngine::instance()->setCallMode(QPulseAudioEngine::CallRinging, AudioModeWiredOrSpeaker);
#endif
}

oFonoConnection::oFonoConnection(const QDBusConnection &dbusConnection,
                            const QString &cmName,
                            const QString &protocolName,
                            const QVariantMap &parameters) :
    Tp::BaseConnection(dbusConnection, cmName, protocolName, parameters),
    mOfonoModemManager(new OfonoModemManager(this)),
    mHandleCount(0),
    mMmsdManager(new MMSDManager(this)),
    mConferenceCall(NULL)
{
    qRegisterMetaType<AudioOutputList>();
    qRegisterMetaType<AudioOutput>();
    OfonoModem::SelectionSetting setting = OfonoModem::AutomaticSelect;
    mModemPath = parameters["modem-objpath"].toString();
    if (!mModemPath.isEmpty()) {
        setting = OfonoModem::ManualSelect;
    }
    mOfonoMessageManager = new OfonoMessageManager(setting, mModemPath);
    mOfonoVoiceCallManager = new OfonoVoiceCallManager(setting, mModemPath);
    mOfonoCallVolume = new OfonoCallVolume(setting, mModemPath);
    mOfonoNetworkRegistration = new OfonoNetworkRegistration(setting, mModemPath);
    mOfonoMessageWaiting = new OfonoMessageWaiting(setting, mModemPath);
    mOfonoSupplementaryServices = new OfonoSupplementaryServices(setting, mModemPath);
    mOfonoSimManager = new OfonoSimManager(setting, mModemPath);
    mOfonoModem = mOfonoSimManager->modem();

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
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialInviteeHandles"));

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
    call.allowedProperties.append(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialChannels"));

    requestsIface->requestableChannelClasses << text << call;

    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(requestsIface));

    // init presence interface
    simplePresenceIface = Tp::BaseConnectionSimplePresenceInterface::create();
    simplePresenceIface->setSetPresenceCallback(Tp::memFun(this,&oFonoConnection::setPresence));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(simplePresenceIface));

    // init custom emergency mode interface (not provided by telepathy
    emergencyModeIface = BaseConnectionEmergencyModeInterface::create();
    emergencyModeIface->setEmergencyNumbersCallback(Tp::memFun(this,&oFonoConnection::emergencyNumbers));
    QObject::connect(mOfonoVoiceCallManager, SIGNAL(emergencyNumbersChanged(QStringList)),
                     emergencyModeIface.data(), SLOT(setEmergencyNumbers(QStringList)));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(emergencyModeIface));

    // init custom voicemail interface (not provided by telepathy)
    voicemailIface = BaseConnectionVoicemailInterface::create();
    voicemailIface->setVoicemailCountCallback(Tp::memFun(this,&oFonoConnection::voicemailCount));
    voicemailIface->setVoicemailIndicatorCallback(Tp::memFun(this,&oFonoConnection::voicemailIndicator));
    voicemailIface->setVoicemailNumberCallback(Tp::memFun(this,&oFonoConnection::voicemailNumber));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(voicemailIface));

    supplementaryServicesIface = BaseConnectionUSSDInterface::create();
    supplementaryServicesIface->setInitiateCallback(Tp::memFun(this,&oFonoConnection::USSDInitiate));
    supplementaryServicesIface->setRespondCallback(Tp::memFun(this,&oFonoConnection::USSDRespond));
    supplementaryServicesIface->setCancelCallback(Tp::memFun(this,&oFonoConnection::USSDCancel));
    supplementaryServicesIface->StateChanged(mOfonoSupplementaryServices->state());
    supplementaryServicesIface->setSerial(mOfonoModem->serial());
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(supplementaryServicesIface));

    // Set Presence
    Tp::SimpleStatusSpec presenceOnline;
    presenceOnline.type = Tp::ConnectionPresenceTypeAvailable;
    presenceOnline.maySetOnSelf = false;
    presenceOnline.canHaveMessage = true;

    Tp::SimpleStatusSpec presenceOffline;
    presenceOffline.type = Tp::ConnectionPresenceTypeOffline;
    presenceOffline.maySetOnSelf = false;
    presenceOffline.canHaveMessage = true;

    Tp::SimpleStatusSpec presenceAway;
    presenceAway.type = Tp::ConnectionPresenceTypeAway;
    presenceAway.maySetOnSelf = false;
    presenceAway.canHaveMessage = true;

    Tp::SimpleStatusSpecMap statuses;
    statuses.insert(QLatin1String("registered"), presenceOnline);
    statuses.insert(QLatin1String("roaming"), presenceOnline);
    statuses.insert(QLatin1String("flightmode"), presenceOffline);
    statuses.insert(QLatin1String("nosim"), presenceOffline);
    statuses.insert(QLatin1String("nomodem"), presenceOffline);
    statuses.insert(QLatin1String("simlocked"), presenceAway);
    statuses.insert(QLatin1String("unregistered"), presenceAway);
    statuses.insert(QLatin1String("denied"), presenceAway);
    statuses.insert(QLatin1String("unknown"), presenceAway);
    statuses.insert(QLatin1String("searching"), presenceAway);

    simplePresenceIface->setStatuses(statuses);

    if (mOfonoModem->isValid()) {
        supplementaryServicesIface->setSerial(mOfonoModem->serial());
    }
    // force update current presence
    updateOnlineStatus();

    contactsIface = Tp::BaseConnectionContactsInterface::create();
    contactsIface->setGetContactAttributesCallback(Tp::memFun(this,&oFonoConnection::getContactAttributes));
    contactsIface->setContactAttributeInterfaces(QStringList()
                                                 << TP_QT_IFACE_CONNECTION
                                                 << TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE);
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(contactsIface));

    QObject::connect(mOfonoModem, SIGNAL(onlineChanged(bool)), SLOT(updateOnlineStatus()));
    QObject::connect(mOfonoModem, SIGNAL(serialChanged(QString)), supplementaryServicesIface.data(), SLOT(setSerial(QString)));
    QObject::connect(mOfonoModem, SIGNAL(interfacesChanged(QStringList)), SLOT(updateOnlineStatus()));
    QObject::connect(mOfonoMessageManager, SIGNAL(incomingMessage(QString,QVariantMap)), this, SLOT(onOfonoIncomingMessage(QString,QVariantMap)));
    QObject::connect(mOfonoMessageManager, SIGNAL(immediateMessage(QString,QVariantMap)), this, SLOT(onOfonoImmediateMessage(QString,QVariantMap)));
    QObject::connect(mOfonoMessageManager, SIGNAL(statusReport(QString,QVariantMap)), this, SLOT(onDeliveryReportReceived(QString,QVariantMap)));
    QObject::connect(mOfonoVoiceCallManager, SIGNAL(callAdded(QString,QVariantMap)), SLOT(onOfonoCallAdded(QString, QVariantMap)));
    QObject::connect(mOfonoSimManager, SIGNAL(validityChanged(bool)), SLOT(onValidityChanged(bool)));
    QObject::connect(mOfonoSimManager, SIGNAL(presenceChanged(bool)), SLOT(updateOnlineStatus()));
    QObject::connect(mOfonoSimManager, SIGNAL(pinRequiredChanged(QString)), SLOT(updateOnlineStatus()));
    QObject::connect(mOfonoNetworkRegistration, SIGNAL(statusChanged(QString)), SLOT(updateOnlineStatus()));
    QObject::connect(mOfonoNetworkRegistration, SIGNAL(nameChanged(QString)), SLOT(updateOnlineStatus()));
    QObject::connect(mOfonoNetworkRegistration, SIGNAL(validityChanged(bool)), SLOT(updateOnlineStatus()));
    QObject::connect(mOfonoMessageWaiting, SIGNAL(voicemailMessageCountChanged(int)), voicemailIface.data(), SLOT(setVoicemailCount(int)));
    QObject::connect(mOfonoMessageWaiting, SIGNAL(voicemailWaitingChanged(bool)), voicemailIface.data(), SLOT(setVoicemailIndicator(bool)));
    QObject::connect(mOfonoMessageWaiting, SIGNAL(voicemailMailboxNumberChanged(QString)), voicemailIface.data(), SLOT(setVoicemailNumber(QString)));

    QObject::connect(mMmsdManager, SIGNAL(serviceAdded(const QString&)), SLOT(onMMSDServiceAdded(const QString&)));
    QObject::connect(mMmsdManager, SIGNAL(serviceRemoved(const QString&)), SLOT(onMMSDServiceRemoved(const QString&)));

    // update audio route
    QObject::connect(mOfonoVoiceCallManager, SIGNAL(callAdded(QString,QVariantMap)), SLOT(updateAudioRoute()));
    QObject::connect(mOfonoVoiceCallManager, SIGNAL(callRemoved(QString)), SLOT(updateAudioRoute()));

#ifdef USE_PULSEAUDIO
    // update audio modes
    QObject::connect(QPulseAudioEngine::instance(), SIGNAL(audioModeChanged(AudioMode)), SLOT(onAudioModeChanged(AudioMode)));
    QObject::connect(QPulseAudioEngine::instance(), SIGNAL(availableAudioModesChanged(AudioModes)), SLOT(onAvailableAudioModesChanged(AudioModes)));
#endif

    QObject::connect(mOfonoSupplementaryServices, SIGNAL(notificationReceived(const QString &)), supplementaryServicesIface.data(), SLOT(NotificationReceived(const QString &)));
    QObject::connect(mOfonoSupplementaryServices, SIGNAL(requestReceived(const QString &)), supplementaryServicesIface.data(), SLOT(RequestReceived(const QString &)));

    QObject::connect(mOfonoSupplementaryServices, SIGNAL(initiateUSSDComplete(const QString &)), supplementaryServicesIface.data(), SLOT(InitiateUSSDComplete(const QString &)));

    QObject::connect(mOfonoSupplementaryServices, SIGNAL(barringComplete(const QString &, const QString &, const QVariantMap &)), 
        supplementaryServicesIface.data(), SLOT(BarringComplete(const QString &, const QString &, const QVariantMap &)));

    QObject::connect(mOfonoSupplementaryServices, SIGNAL(forwardingComplete(const QString &, const QString &, const QVariantMap &)), 
        supplementaryServicesIface.data(), SLOT(ForwardingComplete(const QString &, const QString &, const QVariantMap &)));

    QObject::connect(mOfonoSupplementaryServices, SIGNAL(waitingComplete(const QString &, const QVariantMap &)), 
        supplementaryServicesIface.data(), SLOT(WaitingComplete(const QString &, const QVariantMap &)));

    QObject::connect(mOfonoSupplementaryServices, SIGNAL(callingLinePresentationComplete(const QString &, const QString &)), 
        supplementaryServicesIface.data(), SLOT(CallingLinePresentationComplete(const QString &, const QString &)));

    QObject::connect(mOfonoSupplementaryServices, SIGNAL(connectedLinePresentationComplete(const QString &, const QString &)), 
        supplementaryServicesIface.data(), SLOT(ConnectedLinePresentationComplete(const QString &, const QString &)));

    QObject::connect(mOfonoSupplementaryServices, SIGNAL(callingLineRestrictionComplete(const QString &, const QString &)), 
        supplementaryServicesIface.data(), SLOT(CallingLineRestrictionComplete(const QString &, const QString &)));

    QObject::connect(mOfonoSupplementaryServices, SIGNAL(connectedLineRestrictionComplete(const QString &, const QString &)), 
        supplementaryServicesIface.data(), SLOT(ConnectedLineRestrictionComplete(const QString &, const QString &)));

    QObject::connect(mOfonoSupplementaryServices, SIGNAL(initiateFailed()), supplementaryServicesIface.data(), SLOT(InitiateFailed()));

    QObject::connect(mOfonoSupplementaryServices, SIGNAL(stateChanged(const QString&)), supplementaryServicesIface.data(), SLOT(StateChanged(const QString&)));

    QObject::connect(mOfonoSupplementaryServices, SIGNAL(respondComplete(bool, const QString &)), supplementaryServicesIface.data(), SLOT(RespondComplete(bool, const QString &)));

    // workaround: we can't add services here as tp-ofono interfaces are not exposed on dbus
    // todo: use QDBusServiceWatcher
    QTimer::singleShot(1000, this, SLOT(onCheckMMSServices()));
}

QMap<QString, oFonoCallChannel*> oFonoConnection::callChannels()
{
    return mCallChannels;
}

void oFonoConnection::onCheckMMSServices()
{
    Q_FOREACH(QString servicePath, mMmsdManager->services()) {
        onMMSDServiceAdded(servicePath);
    }
}

void oFonoConnection::onMMSDServiceAdded(const QString &path)
{
    MMSDService *service = new MMSDService(path, this);
    if (service->modemObjectPath() != mModemPath) {
        service->deleteLater();
        return;
    }
    qDebug() << "oFonoConnection::onMMSServiceAdded" << path;
    mMmsdServices[path] = service;
    QObject::connect(service, SIGNAL(messageAdded(const QString&, const QVariantMap&)), SLOT(onMMSAdded(const QString&, const QVariantMap&)));
    QObject::connect(service, SIGNAL(messageRemoved(const QString&)), SLOT(onMMSRemoved(const QString&)));
    Q_FOREACH(MessageStruct message, service->messages()) {
        addMMSToService(message.path.path(), message.properties, service->path());
    }
}

QDBusObjectPath oFonoConnection::sendMMS(const QStringList &numbers, const OutgoingAttachmentList& attachments)
{
    // FIXME: dualsim: mms's for now will only be sent using the first modem
    if (mMmsdServices.count() > 0) {
        return mMmsdServices.first()->sendMessage(numbers, attachments);
    }
    qDebug() << "No mms service available";
    return QDBusObjectPath();
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

oFonoTextChannel* oFonoConnection::textChannelForMembers(const QStringList &members)
{
    Q_FOREACH(oFonoTextChannel* channel, mTextChannels) {
        int count = 0;
        Tp::DBusError error;
        if (members.size() != channel->members().size()) {
            continue;
        }
        QStringList phoneNumbersOld = inspectHandles(Tp::HandleTypeContact, channel->members(), &error);
        if (error.isValid()) {
            continue;
        }

        Q_FOREACH(const QString &phoneNumberNew, members) {
            Q_FOREACH(const QString &phoneNumberOld, phoneNumbersOld) {
                if (PhoneUtils::comparePhoneNumbers(PhoneUtils::normalizePhoneNumber(phoneNumberOld), PhoneUtils::normalizePhoneNumber(phoneNumberNew))) {
                    count++;
                }
            }
        }
        if (count == members.size()) {
            return channel;
        }
    }
    return NULL;
}

void oFonoConnection::addMMSToService(const QString &path, const QVariantMap &properties, const QString &servicePath)
{
    qDebug() << "addMMSToService " << path << properties << servicePath;
    MMSDMessage *msg = new MMSDMessage(path, properties);
    mServiceMMSList[servicePath].append(msg);
    if (properties["Status"] ==  "received") {
        const QString normalizedNumber = PhoneUtils::normalizePhoneNumber(properties["Sender"].toString());
        // check if there is an open channel for this number and use it
        oFonoTextChannel *channel = textChannelForMembers(QStringList() << normalizedNumber);
        if (channel) {
            channel->mmsReceived(path, ensureHandle(normalizedNumber), properties);
            return;
        }

        Tp::DBusError error;
        bool yours;
        qDebug() << "new handle" << normalizedNumber;
        uint handle = newHandle(normalizedNumber);
        ensureChannel(TP_QT_IFACE_CHANNEL_TYPE_TEXT,Tp::HandleTypeContact, handle, yours, handle, false, QVariantMap(), &error);
        if(error.isValid()) {
            qCritical() << "Error creating channel for incoming message " << error.name() << error.message();
            return;
        }
        channel = textChannelForMembers(QStringList() << normalizedNumber);
        if (channel) {
            channel->mmsReceived(path, ensureHandle(normalizedNumber), properties);
        }
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
    Q_FOREACH(MMSDService *service, mMmsdServices) {
        onMMSDServiceRemoved(service->path());
    }
   
}

bool oFonoConnection::isNetworkRegistered()
{
    QString status = mOfonoNetworkRegistration->status();
    if (!mOfonoNetworkRegistration->isValid()) {
        return false;
    }
    return  !(!mOfonoModem->isValid() ||
              !mOfonoModem->online() ||
              status == "unregistered" ||
              status == "denied" ||
              status == "unknown" ||
              status == "searching" ||
              status.isEmpty());
}

bool oFonoConnection::isEmergencyNumber(const QString &number)
{
    Q_FOREACH (const QString &emergencyNumber, mOfonoVoiceCallManager->emergencyNumbers()) {
        if (PhoneUtils::comparePhoneNumbers(emergencyNumber, number)) {
            return true;
            break;
        }
    }
    return false;
}

uint oFonoConnection::setPresence(const QString& status, const QString& statusMessage, Tp::DBusError *error)
{
    qDebug() << "setPresence" << status;
    // this prevents tp-qt to propagate the available status
    error->set(TP_QT_ERROR_NOT_AVAILABLE, "Can't change online status: Operation not supported");
    return selfHandle();
}

Tp::ContactAttributesMap oFonoConnection::getContactAttributes(const Tp::UIntList &handles, const QStringList &ifaces, Tp::DBusError *error)
{
    qDebug() << "getContactAttributes" << handles << ifaces;
    Tp::ContactAttributesMap attributesMap;
    QVariantMap attributes;
    Q_FOREACH(uint handle, handles) {
        QStringList inspectedHandles = inspectHandles(Tp::HandleTypeContact, Tp::UIntList() << handle, error);
        if (inspectedHandles.size() > 0) {
            attributes[TP_QT_IFACE_CONNECTION+"/contact-id"] = inspectedHandles.at(0);
        } else {
            continue;
        }
        if (ifaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE)) {
            attributes[TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE+"/presence"] = QVariant::fromValue(mSelfPresence);
        }
        attributesMap[handle] = attributes;
    }
    return attributesMap;
}

void oFonoConnection::onValidityChanged(bool valid)
{
    // WORKAROUND: ofono-qt does not refresh the properties once the interface
    // becomes available, so it contains old values.
    Q_EMIT mOfonoSimManager->modem()->pathChanged(mOfonoModem->path());
    Q_EMIT mOfonoNetworkRegistration->modem()->pathChanged(mOfonoModem->path());
    QString modemSerial;
    if (valid) {
        modemSerial = mOfonoModem->serial();
    }
    supplementaryServicesIface->setSerial(modemSerial);
    updateOnlineStatus();
}

void oFonoConnection::updateOnlineStatus()
{
    Tp::SimpleContactPresences presences;
    mSelfPresence.statusMessage = "";
    mSelfPresence.type = Tp::ConnectionPresenceTypeOffline;

    if (!mOfonoModem->isValid()) {
        mSelfPresence.status = "nomodem";
    } else if (!mOfonoModem->online()) {
        mSelfPresence.status = "flightmode";
    } else if ((mOfonoSimManager->isValid() && !mOfonoSimManager->present()) ||
                !mOfonoSimManager->isValid()) {
        mSelfPresence.status = "nosim";
    } else if (mOfonoSimManager->pinRequired() != "none") {
        mSelfPresence.status = "simlocked";
        mSelfPresence.type = Tp::ConnectionPresenceTypeAway;
    } else if (isNetworkRegistered()) {
        mSelfPresence.status = mOfonoNetworkRegistration->status();
        mSelfPresence.statusMessage = mOfonoNetworkRegistration->name();
        mSelfPresence.type = Tp::ConnectionPresenceTypeAvailable;
    } else {
        mSelfPresence.status = mOfonoNetworkRegistration->status();
        mSelfPresence.type = Tp::ConnectionPresenceTypeAway;
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
        const QString normalizedNumber = PhoneUtils::normalizePhoneNumber(identifier);
        if (mHandles.values().contains(normalizedNumber)) {
            handles.append(mHandles.key(normalizedNumber));
        } else if (PhoneUtils::isPhoneNumber(normalizedNumber)) {
            handles.append(newHandle(normalizedNumber));
        } else {
            handles.append(newHandle(identifier));
        }
    }
    qDebug() << "requestHandles" << handles;
    return handles;
}

Tp::BaseChannelPtr oFonoConnection::createTextChannel(uint targetHandleType,
                                               uint targetHandle, const QVariantMap &hints, Tp::DBusError *error)
{
    Q_UNUSED(targetHandleType);

    if (mSelfPresence.type != Tp::ConnectionPresenceTypeAvailable) {
        error->set(TP_QT_ERROR_NETWORK_ERROR, "No network available");
        return Tp::BaseChannelPtr();
    }

    QStringList phoneNumbers;
    bool flash = false;
    if (hints.contains(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialInviteeHandles"))) {
        phoneNumbers << inspectHandles(Tp::HandleTypeContact, qdbus_cast<Tp::UIntList>(hints[TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialInviteeHandles")]), error);
    } else {
        phoneNumbers << mHandles.value(targetHandle);
    }

    if (hints.contains(TP_QT_IFACE_CHANNEL_INTERFACE_SMS + QLatin1String(".Flash"))) {
        flash = hints[TP_QT_IFACE_CHANNEL_INTERFACE_SMS + QLatin1String(".Flash")].toBool();
    }

    oFonoTextChannel *channel = new oFonoTextChannel(this, phoneNumbers, flash);
    mTextChannels << channel;
    QObject::connect(channel, SIGNAL(messageRead(QString)), SLOT(onMessageRead(QString)));
    QObject::connect(channel, SIGNAL(destroyed()), SLOT(onTextChannelClosed()));
    return channel->baseChannel();
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

void oFonoConnection::onConferenceCallChannelClosed()
{
    if (mConferenceCall) {
        mConferenceCall = NULL;
    }
}

Tp::BaseChannelPtr oFonoConnection::createCallChannel(uint targetHandleType,
                                               uint targetHandle, const QVariantMap &hints, Tp::DBusError *error)
{
    Q_UNUSED(targetHandleType);

    bool success = true;
    QString newPhoneNumber = mHandles.value(targetHandle);
    bool available = (mSelfPresence.type == Tp::ConnectionPresenceTypeAvailable);
    bool isConference = (hints.contains(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialChannels")) &&
                         targetHandleType == Tp::HandleTypeNone && targetHandle == 0);

    if (!available && (isConference || !isEmergencyNumber(newPhoneNumber))) {
        error->set(TP_QT_ERROR_NETWORK_ERROR, "No network available");
        return Tp::BaseChannelPtr();
    }

    if (isConference) {
        // conference call request
        if (mConferenceCall) {
            error->set(TP_QT_ERROR_NOT_AVAILABLE, "Conference call already exists");
            return Tp::BaseChannelPtr();
        }

        QList<QDBusObjectPath> channels = mOfonoVoiceCallManager->createMultiparty();
        if (!channels.isEmpty()) {
            mConferenceCall = new oFonoConferenceCallChannel(this);
            QObject::connect(mConferenceCall, SIGNAL(destroyed()), SLOT(onConferenceCallChannelClosed()));
            return mConferenceCall->baseChannel();
        }
        error->set(TP_QT_ERROR_NOT_AVAILABLE, "Impossible to merge calls");
        return Tp::BaseChannelPtr();
    }

    QDBusObjectPath objpath(hints["ofonoObjPath"].toString());

    if (objpath.path().isEmpty()) {
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

    oFonoCallChannel *channel = new oFonoCallChannel(this, newPhoneNumber, targetHandle, objpath.path());
    mCallChannels[objpath.path()] = channel;
    QObject::connect(channel, SIGNAL(destroyed()), SLOT(onCallChannelDestroyed()));
    QObject::connect(channel, SIGNAL(closed()), SLOT(onCallChannelClosed()));
    QObject::connect(channel, SIGNAL(merged()), SLOT(onCallChannelMerged()));
    QObject::connect(channel, SIGNAL(splitted()), SLOT(onCallChannelSplitted()));
    QObject::connect(channel, SIGNAL(multipartyCallHeld()), SLOT(onMultipartyCallHeld()));
    QObject::connect(channel, SIGNAL(multipartyCallActive()), SLOT(onMultipartyCallActive()));
    qDebug() << channel;
    return channel->baseChannel();
}

void oFonoConnection::onMultipartyCallHeld()
{
    if (!mConferenceCall) {
        return;
    }

    mConferenceCall->setConferenceActive(false);
}

void oFonoConnection::onMultipartyCallActive()
{
    if (!mConferenceCall) {
        return;
    }

    mConferenceCall->setConferenceActive(true);
}

void oFonoConnection::onCallChannelMerged()
{
    if (!mConferenceCall) {
        return;
    }
    oFonoCallChannel *channel = static_cast<oFonoCallChannel*>(sender());
    Q_EMIT channelMerged(QDBusObjectPath(channel->baseChannel()->objectPath()));
}

void oFonoConnection::onCallChannelSplitted()
{
    if (!mConferenceCall) {
        return;
    }
    oFonoCallChannel *channel = static_cast<oFonoCallChannel*>(sender());
    Q_EMIT channelSplitted(QDBusObjectPath(channel->baseChannel()->objectPath()));
}


Tp::BaseChannelPtr oFonoConnection::createChannel(const QString& channelType, uint targetHandleType,
                                               uint targetHandle, const QVariantMap &hints, Tp::DBusError *error)
{
    if (channelType == TP_QT_IFACE_CHANNEL_TYPE_TEXT) {
        return createTextChannel(targetHandleType, targetHandle, hints, error);
    } else if (channelType == TP_QT_IFACE_CHANNEL_TYPE_CALL) {
        return createCallChannel(targetHandleType, targetHandle, hints, error);
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

void oFonoConnection::onDeliveryReportReceived(const QString &messageId, const QVariantMap& info)
{
    const QString pendingMessageNumber = PendingMessagesManager::instance()->recipientIdForMessageId(messageId);
    if (pendingMessageNumber.isEmpty()) {
        return;
    }
    const QString normalizedNumber = PhoneUtils::normalizePhoneNumber(pendingMessageNumber);
    PendingMessagesManager::instance()->removePendingMessage(messageId);
    // check if there is an open channel for this sender and use it

    oFonoTextChannel *channel = textChannelForMembers(QStringList() << normalizedNumber);
    if(channel) {
        channel->deliveryReportReceived(messageId, ensureHandle(normalizedNumber), info["Delivered"].toBool());
        return;
    }

    Tp::DBusError error;
    bool yours;
    uint handle = newHandle(normalizedNumber);
    ensureChannel(TP_QT_IFACE_CHANNEL_TYPE_TEXT,Tp::HandleTypeContact, handle, yours, handle, false, QVariantMap(), &error);
    if(error.isValid()) {
        qWarning() << "Error creating channel for incoming message" << error.name() << error.message();
        return;
    }
    channel = textChannelForMembers(QStringList() << normalizedNumber);
    if(channel) {
        channel->deliveryReportReceived(messageId, ensureHandle(normalizedNumber), info["Delivered"].toBool());
        return;
    }
}

void oFonoConnection::onOfonoIncomingMessage(const QString &message, const QVariantMap &info)
{
    ensureTextChannel(message, info, false);
}

void oFonoConnection::onOfonoImmediateMessage(const QString &message, const QVariantMap &info)
{
    ensureTextChannel(message, info, true);
}

void oFonoConnection::ensureTextChannel(const QString &message, const QVariantMap &info, bool flash)
{
    const QString normalizedNumber = PhoneUtils::normalizePhoneNumber(info["Sender"].toString());
    // check if there is an open channel for this sender and use it
    oFonoTextChannel *channel = textChannelForMembers(QStringList() << normalizedNumber);
    if(channel) {
        channel->messageReceived(message, ensureHandle(normalizedNumber), info);
        return;
    }

    Tp::DBusError error;
    bool yours;
    QVariantMap hints;
    hints[TP_QT_IFACE_CHANNEL_INTERFACE_SMS + QLatin1String(".Flash")] = flash;
    uint handle = newHandle(normalizedNumber);
    ensureChannel(TP_QT_IFACE_CHANNEL_TYPE_TEXT,Tp::HandleTypeContact, handle, yours, handle, false, hints, &error);
    if(error.isValid()) {
        qWarning() << "Error creating channel for incoming message" << error.name() << error.message();
        return;
    }

    channel = textChannelForMembers(QStringList() << normalizedNumber);
    if (channel) {
        channel->messageReceived(message, ensureHandle(normalizedNumber), info);
    }
}

void oFonoConnection::onTextChannelClosed()
{
    oFonoTextChannel *channel = static_cast<oFonoTextChannel*>(sender());
    if (channel) {
        qDebug() << "text channel closed";
        mTextChannels.removeAll(channel);
    }
}

void oFonoConnection::onCallChannelClosed()
{
    qDebug() << "onCallChannelClosed()";
    oFonoCallChannel *channel = static_cast<oFonoCallChannel*>(sender());
    if (channel) {
        Q_EMIT channelHangup(QDBusObjectPath(channel->baseChannel()->objectPath()));
    }
}


void oFonoConnection::onCallChannelDestroyed()
{
    qDebug() << "onCallChannelDestroyed()";
    oFonoCallChannel *channel = static_cast<oFonoCallChannel*>(sender());
    if (channel) {
        QString key = mCallChannels.key(channel);
        qDebug() << "call channel closed for number " << key;
        mCallChannels.remove(key);
    }
}

uint oFonoConnection::ensureHandle(const QString &phoneNumber)
{
    const QString normalizedNumber = PhoneUtils::normalizePhoneNumber(phoneNumber);

    Q_FOREACH(const QString &phone, mHandles.values()) {
        if (PhoneUtils::comparePhoneNumbers(normalizedNumber, phone)) {
            // this user already exists
            return mHandles.key(phone);
        }
    }
    return newHandle(normalizedNumber);
}

Tp::BaseChannelPtr oFonoConnection::ensureChannel(const QString &channelType, uint targetHandleType,
        uint targetHandle, bool &yours, uint initiatorHandle,
        bool suppressHandler,
        const QVariantMap &hints,
        Tp::DBusError* error)
{
    // we only reuse old text channels
    if (channelType == TP_QT_IFACE_CHANNEL_TYPE_TEXT) {
        Q_FOREACH(oFonoTextChannel *channel, mTextChannels) {
            if (channel->baseChannel()->targetHandleType() == targetHandleType
                    && channel->baseChannel()->targetHandle() == targetHandle) {
                yours = false;
                return channel->baseChannel();
            }
        }
    }
    yours = true;
    return Tp::BaseConnection::createChannel(channelType, targetHandleType, targetHandle, initiatorHandle, suppressHandler, hints, error);
}

void oFonoConnection::onOfonoCallAdded(const QString &call, const QVariantMap &properties)
{
    qDebug() << "new call" << call << properties;

    bool yours;
    Tp::DBusError error;
    QString lineIdentification = properties["LineIdentification"].toString();

    // check if there is an open channel for this call, if so, ignore it
    if (mCallChannels.keys().contains(call)) {
        qWarning() << "call channel for this object path already exists: " << call;
        return;
    }

    QString normalizedNumber;
    // TODO check if more than one private/unknown calls are supported at the same time
    if (lineIdentification.isEmpty()) {
        // unknown caller Id
        lineIdentification = QString("x-ofono-unknown");
        normalizedNumber = lineIdentification;
    } else if (lineIdentification == "withheld") {
        // private caller
        lineIdentification = QString("x-ofono-private");
        normalizedNumber = lineIdentification;
    } else {
        normalizedNumber = PhoneUtils::normalizePhoneNumber(lineIdentification);
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

    QVariantMap hints;
    hints["ofonoObjPath"] = call;
    Tp::BaseChannelPtr channel  = ensureChannel(TP_QT_IFACE_CHANNEL_TYPE_CALL, Tp::HandleTypeContact, handle, yours, initiatorHandle, false, hints, &error);
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

QString oFonoConnection::activeAudioOutput()
{
    return mActiveAudioOutput;
}

AudioOutputList oFonoConnection::audioOutputs()
{
    return mAudioOutputs;
}

QStringList oFonoConnection::emergencyNumbers(Tp::DBusError *error)
{
    return mOfonoVoiceCallManager->emergencyNumbers();
}

void oFonoConnection::setActiveAudioOutput(const QString &id)
{
    mActiveAudioOutput = id;
    Q_EMIT activeAudioOutputChanged(id);
}

void oFonoConnection::USSDInitiate(const QString &command, Tp::DBusError *error)
{
    mOfonoSupplementaryServices->initiate(command);
}

void oFonoConnection::USSDRespond(const QString &reply, Tp::DBusError *error)
{
    mOfonoSupplementaryServices->respond(reply);
}

void oFonoConnection::USSDCancel(Tp::DBusError *error)
{
    mOfonoSupplementaryServices->cancel();
}

#ifdef USE_PULSEAUDIO
void oFonoConnection::onAudioModeChanged(AudioMode mode)
{
    qDebug("PulseAudio audio mode changed: 0x%x", mode);

    if ((mode == AudioModeEarpiece && mActiveAudioOutput != "default") ||
        (mode == AudioModeWiredHeadset && mActiveAudioOutput != "default")) {
        setActiveAudioOutput("default");
    } else if (mode == AudioModeSpeaker && mActiveAudioOutput != "speaker") {
        setActiveAudioOutput("speaker");
    } else if (mode == AudioModeBluetooth && mActiveAudioOutput != "bluetooth") {
        setActiveAudioOutput("bluetooth");
    }
}

void oFonoConnection::onAvailableAudioModesChanged(AudioModes modes)
{
    qDebug("PulseAudio available audio modes changed");
    bool defaultFound = false;
    mAudioOutputs.clear();
    Q_FOREACH(const AudioMode &mode, modes) {
        AudioOutput output;
        if (mode == AudioModeBluetooth) {
            // there can be only one bluetooth
            output.id = "bluetooth";
            output.type = "bluetooth";
            // we dont support names for now, so we set a default value
            output.name = "bluetooth";
        } else if (mode == AudioModeEarpiece || mode == AudioModeWiredHeadset) {
            if (!defaultFound) {
                defaultFound = true;
                output.id = "default";
                output.type = "default";
                output.name = "default";
            } else {
                continue;
            }
        } else if (mode == AudioModeSpeaker) {
            output.id = "speaker";
            output.type = "speaker";
            output.name = "speaker";
        }
        mAudioOutputs << output;
    }
    Q_EMIT audioOutputsChanged(mAudioOutputs);
}
#endif

void oFonoConnection::updateAudioRoute()
{
    QByteArray pulseAudioDisabled = qgetenv("PA_DISABLED");
    if (!pulseAudioDisabled.isEmpty()) {
        return;
    }

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
                if (call->state() == "disconnected") {
                    enable_normal();
                    call->deleteLater();
                    return;
                }
                // if only one call and dialing, default to earpiece
                if (call->state() == "dialing") {
                    enable_earpiece();
                    call->deleteLater();
                    return;
                }
                if (call->state().isEmpty()) {
                    call->deleteLater();
                    return;
                }
                call->deleteLater();
            }
        }
    } else {
        enable_normal();
    }

}

// this method is only called when call channels go from incoming to active.
// please call this method only from oFonoCallChannel instances
void oFonoConnection::updateAudioRouteToEarpiece()
{
    if (mOfonoVoiceCallManager->getCalls().size() == 1) {
        enable_earpiece();
    }
}
