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

// qt
#include <QTimer>

// telepathy-qt
#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/BaseCall>
#include <TelepathyQt/AbstractAdaptor>
#include <TelepathyQt/DBusError>

// ofono-qt
#include <ofonomodemmanager.h>
#include <ofonomessagemanager.h>
#include <ofonovoicecallmanager.h>
#include <ofonovoicecall.h>
#include <ofonocallvolume.h>
#include <ofononetworkregistration.h>
#include <ofonomessagewaiting.h>
#include <ofonosupplementaryservices.h>

// telepathy-ofono
#include "ofonotextchannel.h"
#include "ofonocallchannel.h"
#include "voicemailiface.h"
#include "mmsdmanager.h"
#include "mmsdmessage.h"
#include "dbustypes.h"
#include "speakeriface.h"
#include "ussdiface.h"

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
                                     uint targetHandle,  const QVariantMap &hints, Tp::DBusError *error);
    Tp::ContactAttributesMap getContactAttributes(const Tp::UIntList &handles, const QStringList &ifaces, Tp::DBusError *error);
    uint setPresence(const QString& status, const QString& statusMessage, Tp::DBusError *error);
    void connect(Tp::DBusError *error);
    void setOnline(bool online);
    void setSpeakerMode(bool active);
    bool speakerMode();
    bool voicemailIndicator(Tp::DBusError *error);
    QString voicemailNumber(Tp::DBusError *error);
    uint voicemailCount(Tp::DBusError *error);
    void USSDInitiate(const QString &command, Tp::DBusError *error);
    void USSDRespond(const QString &reply, Tp::DBusError *error);
    void USSDCancel(Tp::DBusError *error);

    Tp::BaseConnectionRequestsInterfacePtr requestsIface;
    Tp::BaseConnectionSimplePresenceInterfacePtr simplePresenceIface;
    Tp::BaseConnectionContactsInterfacePtr contactsIface;
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
                                         uint targetHandle,  const QVariantMap &hints, Tp::DBusError *error);
    Tp::BaseChannelPtr createCallChannel(uint targetHandleType,
                                         uint targetHandle,  const QVariantMap &hints, Tp::DBusError *error);

    ~oFonoConnection();

Q_SIGNALS:
    void speakerModeChanged(bool active);
    void channelMerged(const QDBusObjectPath &objPath);
    void channelSplitted(const QDBusObjectPath &objPath);
    void channelHangup(const QDBusObjectPath &objPath);

public Q_SLOTS:
    void Q_DBUS_EXPORT onTryRegister();
    void updateAudioRoute();

private Q_SLOTS:
    void onOfonoIncomingMessage(const QString &message, const QVariantMap &info);
    void onOfonoCallAdded(const QString &call, const QVariantMap &properties);
    void onOfonoNetworkRegistrationChanged(const QString &status);
    void onTextChannelClosed();
    void onCallChannelClosed();
    void onCallChannelDestroyed();
    void onValidityChanged(bool valid);
    void onMMSDServiceAdded(const QString&);
    void onMMSDServiceRemoved(const QString&);
    void onMMSAdded(const QString &, const QVariantMap&);
    void onMMSRemoved(const QString &);
    void onMMSPropertyChanged(QString property, QVariant value);
    void onCheckMMSServices();
    void onMessageRead(const QString &id);
    void onDeliveryReportReceived(const QString &messageId, const QVariantMap &info);
    void onConferenceCallChannelClosed();
    void onCallChannelMerged();
    void onCallChannelSplitted();
    void onMultipartyCallHeld();
    void onMultipartyCallActive();

private:
    bool isNetworkRegistered();
    void addMMSToService(const QString &path, const QVariantMap &properties, const QString &servicePath);
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
    uint mHandleCount;
    Tp::SimplePresence mSelfPresence;
    Tp::SimplePresence mRequestedSelfPresence;
    QTimer *mRegisterTimer;
    MMSDManager *mMmsdManager;
    QMap<QString, MMSDService*> mMmsdServices;
    QMap<QString, QList<MMSDMessage*> > mServiceMMSList;
    oFonoConferenceCallChannel *mConferenceCall;
    bool mSpeakerMode;
};

#endif
