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

#ifndef OFONOCONNECTION_H
#define OFONOCONNECTION_H

// telepathy-qt
#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/BaseCall>
#include <TelepathyQt/AbstractAdaptor>
#include <TelepathyQt/DBusError>

// ofono-qt
#include <ofonomodem.h>
#include <ofonomodemmanager.h>
#include <ofonomessagemanager.h>
#include <ofonovoicecallmanager.h>
#include <ofonovoicecall.h>
#include <ofonocallvolume.h>
#include <ofononetworkregistration.h>
#include <ofonomessagewaiting.h>
#include <ofonosupplementaryservices.h>
#include <ofonosimmanager.h>

// telepathy-ofono
#include "ofonotextchannel.h"
#include "ofonocallchannel.h"
#include "emergencymodeiface.h"
#include "voicemailiface.h"
#include "mmsdmanager.h"
#include "mmsdmessage.h"
#include "dbustypes.h"
#include "audiooutputsiface.h"
#include "ussdiface.h"

#ifdef USE_PULSEAUDIO
#include "qpulseaudioengine.h"
#endif

class oFonoConnection;
class oFonoTextChannel;
class oFonoCallChannel;
class oFonoConferenceCallChannel;
class MMSDService;

class oFonoConnection : public Tp::BaseConnection
{
    Q_OBJECT
    Q_DISABLE_COPY(oFonoConnection)
public:
    oFonoConnection(const QDBusConnection &dbusConnection,
                    const QString &cmName,
                    const QString &protocolName,
                    const QVariantMap &parameters);

    QStringList inspectHandles(uint handleType, const Tp::UIntList& handles, Tp::DBusError *error);
    Tp::UIntList requestHandles(uint handleType, const QStringList& identifiers, Tp::DBusError* error);
    Tp::BaseChannelPtr createChannel(const QString& channelType, uint targetHandleType,
                                     uint targetHandle, const QVariantMap &hints, Tp::DBusError *error);
    Tp::ContactAttributesMap getContactAttributes(const Tp::UIntList &handles, const QStringList &ifaces, Tp::DBusError *error);
    uint setPresence(const QString& status, const QString& statusMessage, Tp::DBusError *error);
    void connect(Tp::DBusError *error);
    void setSpeakerMode(bool active);
    void setActiveAudioOutput(const QString &id);
    AudioOutputList audioOutputs();
    QString activeAudioOutput();
    QStringList emergencyNumbers(Tp::DBusError *error);
    bool voicemailIndicator(Tp::DBusError *error);
    QString voicemailNumber(Tp::DBusError *error);
    uint voicemailCount(Tp::DBusError *error);
    void USSDInitiate(const QString &command, Tp::DBusError *error);
    void USSDRespond(const QString &reply, Tp::DBusError *error);
    void USSDCancel(Tp::DBusError *error);

    Tp::BaseConnectionRequestsInterfacePtr requestsIface;
    Tp::BaseConnectionSimplePresenceInterfacePtr simplePresenceIface;
    Tp::BaseConnectionContactsInterfacePtr contactsIface;
    BaseConnectionEmergencyModeInterfacePtr emergencyModeIface;
    BaseConnectionVoicemailInterfacePtr voicemailIface;
    BaseConnectionUSSDInterfacePtr supplementaryServicesIface;
    uint newHandle(const QString &identifier);

    OfonoMessageManager *messageManager();
    OfonoVoiceCallManager *voiceCallManager();
    OfonoCallVolume *callVolume();
    QMap<QString, oFonoCallChannel*> callChannels();

    uint ensureHandle(const QString &phoneNumber);
    oFonoTextChannel* textChannelForMembers(const QStringList &members);
    Tp::BaseChannelPtr createTextChannel(uint targetHandleType,
                                         uint targetHandle, const QVariantMap &hints, Tp::DBusError *error);
    Tp::BaseChannelPtr createCallChannel(uint targetHandleType,
                                         uint targetHandle, const QVariantMap &hints, Tp::DBusError *error);
    Tp::BaseChannelPtr ensureChannel(const QString &channelType, uint targetHandleType,
        uint targetHandle, bool &yours, uint initiatorHandle,
        bool suppressHandler,
        const QVariantMap &hints,
        Tp::DBusError* error);
    QDBusObjectPath sendMMS(const QStringList &numbers, const OutgoingAttachmentList& attachments);


    ~oFonoConnection();

Q_SIGNALS:
    void activeAudioOutputChanged(const QString &id);
    void audioOutputsChanged(const AudioOutputList &outputs);
    void channelMerged(const QDBusObjectPath &objPath);
    void channelSplitted(const QDBusObjectPath &objPath);
    void channelHangup(const QDBusObjectPath &objPath);

public Q_SLOTS:
    void updateAudioRoute();
    void updateAudioRouteToEarpiece();

private Q_SLOTS:
    void onOfonoIncomingMessage(const QString &message, const QVariantMap &info);
    void onOfonoImmediateMessage(const QString &message, const QVariantMap &info);
    void onOfonoCallAdded(const QString &call, const QVariantMap &properties);
    void onTextChannelClosed();
    void onCallChannelClosed();
    void onCallChannelDestroyed();
    void onValidityChanged(bool valid);
    void onMMSDServiceAdded(const QString&);
    void onMMSDServiceRemoved(const QString&);
    void onMMSAdded(const QString &, const QVariantMap&);
    void onMMSRemoved(const QString &);
    void onCheckMMSServices();
    void onMessageRead(const QString &id);
    void onDeliveryReportReceived(const QString &messageId, const QVariantMap &info);
    void onConferenceCallChannelClosed();
    void onCallChannelMerged();
    void onCallChannelSplitted();
    void onMultipartyCallHeld();
    void onMultipartyCallActive();
    void updateOnlineStatus();

#ifdef USE_PULSEAUDIO
    void onAudioModeChanged(AudioMode mode);
    void onAvailableAudioModesChanged(AudioModes modes);
#endif

private:
    bool isNetworkRegistered();
    bool isEmergencyNumber(const QString &number);
    void addMMSToService(const QString &path, const QVariantMap &properties, const QString &servicePath);
    void ensureTextChannel(const QString &message, const QVariantMap &info, bool flash);
    QMap<uint, QString> mHandles;

    QList<oFonoTextChannel*> mTextChannels;
    QMap<QString, oFonoCallChannel*> mCallChannels;

    QStringList mModems;
    OfonoModemManager *mOfonoModemManager;
    OfonoMessageManager *mOfonoMessageManager;
    OfonoVoiceCallManager *mOfonoVoiceCallManager;
    OfonoCallVolume *mOfonoCallVolume;
    OfonoNetworkRegistration *mOfonoNetworkRegistration;
    OfonoMessageWaiting *mOfonoMessageWaiting;
    OfonoSupplementaryServices *mOfonoSupplementaryServices;
    OfonoSimManager *mOfonoSimManager;
    OfonoModem *mOfonoModem;
    uint mHandleCount;
    Tp::SimplePresence mSelfPresence;
    MMSDManager *mMmsdManager;
    QMap<QString, MMSDService*> mMmsdServices;
    QMap<QString, QList<MMSDMessage*> > mServiceMMSList;
    oFonoConferenceCallChannel *mConferenceCall;
    QString mModemPath;
    QString mActiveAudioOutput;
    AudioOutputList mAudioOutputs;
};

#endif
