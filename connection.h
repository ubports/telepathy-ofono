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

// telepathy-ofono
#include "ofonotextchannel.h"
#include "ofonocallchannel.h"
#include "voicemailiface.h"

class oFonoConnection;
class oFonoTextChannel;
class oFonoCallChannel;

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
                                     uint targetHandle, Tp::DBusError *error);
    Tp::ContactAttributesMap getContactAttributes(const Tp::UIntList &handles, const QStringList &ifaces, Tp::DBusError *error);
    uint setPresence(const QString& status, const QString& statusMessage, Tp::DBusError *error);
    void connect(Tp::DBusError *error);
    void setOnline(bool online);
    bool voicemailIndicator(Tp::DBusError *error);
    QString voicemailNumber(Tp::DBusError *error);
    uint voicemailCount(Tp::DBusError *error);

    Tp::BaseConnectionRequestsInterfacePtr requestsIface;
    Tp::BaseConnectionSimplePresenceInterfacePtr simplePresenceIface;
    Tp::BaseConnectionContactsInterfacePtr contactsIface;
    BaseConnectionVoicemailInterfacePtr voicemailIface;
    uint newHandle(const QString &identifier);

    OfonoMessageManager *messageManager();
    OfonoVoiceCallManager *voiceCallManager();
    OfonoCallVolume *callVolume();

    uint ensureHandle(const QString &phoneNumber);
    Tp::BaseChannelPtr createTextChannel(uint targetHandleType,
                                         uint targetHandle, Tp::DBusError *error);
    Tp::BaseChannelPtr createCallChannel(uint targetHandleType,
                                         uint targetHandle, Tp::DBusError *error);

    ~oFonoConnection();
public Q_SLOTS:
    void Q_DBUS_EXPORT onTryRegister();

private Q_SLOTS:
    void onOfonoIncomingMessage(const QString &message, const QVariantMap &info);
    void onOfonoCallAdded(const QString &call, const QVariantMap &properties);
    void onOfonoNetworkRegistrationChanged(const QString &status);
    void onTextChannelClosed();
    void onCallChannelClosed();
    void onValidityChanged(bool valid);


private:
    bool isNetworkRegistered();
    QMap<uint, QString> mHandles;

    QMap<QString, oFonoTextChannel*> mTextChannels;
    QMap<QString, oFonoCallChannel*> mCallChannels;

    QStringList mModems;
    OfonoModemManager *mOfonoModemManager;
    OfonoMessageManager *mOfonoMessageManager;
    OfonoVoiceCallManager *mOfonoVoiceCallManager;
    OfonoCallVolume *mOfonoCallVolume;
    OfonoNetworkRegistration *mOfonoNetworkRegistration;
    OfonoMessageWaiting *mOfonoMessageWaiting;
    uint mHandleCount;
    Tp::SimplePresence mSelfPresence;
    Tp::SimplePresence mRequestedSelfPresence;
    QTimer *mRegisterTimer;
};

#endif
