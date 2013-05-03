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

#ifndef OFONOCONNECTION_H
#define OFONOCONNECTION_H

// telepathy-qt
#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/BaseCall>

// ofono-qt
#include <ofonomodemmanager.h>
#include <ofonomessagemanager.h>
#include <ofonovoicecallmanager.h>
#include <ofonovoicecall.h>
#include <ofonocallvolume.h>

// telepathy-ofono
#include "ofonotextchannel.h"
#include "ofonocallchannel.h"


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
    void connect(Tp::DBusError *error);

    Tp::BaseConnectionRequestsInterfacePtr requestsIface;
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

private Q_SLOTS:
    void onOfonoIncomingMessage(const QString &message, const QVariantMap &info);
    void onOfonoCallAdded(const QString &call, const QVariantMap &properties);
    void onTextChannelClosed();
    void onCallChannelClosed();

private:
    QMap<uint, QString> mHandles;

    QMap<QString, oFonoTextChannel*> mTextChannels;
    QMap<QString, oFonoCallChannel*> mCallChannels;

    QStringList mModems;
    OfonoModemManager *mOfonoModemManager;
    OfonoMessageManager *mOfonoMessageManager;
    OfonoVoiceCallManager *mOfonoVoiceCallManager;
    OfonoCallVolume *mOfonoCallVolume;
    uint mHandleCount;
};

#endif
