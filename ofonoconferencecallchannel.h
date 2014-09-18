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

#ifndef OFONOCONFERENCECALLCHANNEL_H
#define OFONOCONFERENCECALLCHANNEL_H

#include <QObject>

#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/BaseCall>
#include <TelepathyQt/Types>

#include <ofono-qt/ofonovoicecall.h>

#include "connection.h"
#include "audiooutputsiface.h"

class oFonoConnection;

class oFonoConferenceCallChannel : public QObject
{
    Q_OBJECT
public:
    oFonoConferenceCallChannel(oFonoConnection *conn, QObject *parent = 0);
    ~oFonoConferenceCallChannel();

    void onHangup(uint reason, const QString &detailedReason, const QString &message, Tp::DBusError* error);
    void onMuteStateChanged(const Tp::LocalMuteState &state, Tp::DBusError *error);
    void onHoldStateChanged(const Tp::LocalHoldState &state, const Tp::LocalHoldStateReason &reason, Tp::DBusError *error);
    void onDTMFStartTone(uchar event, Tp::DBusError *error);
    void onDTMFStopTone(Tp::DBusError *error);
    void onSetActiveAudioOutput(const QString &id, Tp::DBusError *error);
    void onMerge(const QDBusObjectPath &channel, Tp::DBusError *error);
    Tp::BaseChannelPtr baseChannel();
    void setConferenceActive(bool active);

private Q_SLOTS:
    void onDtmfComplete(bool success);
    void sendNextDtmf();
    void init();

    void onOfonoMuteChanged(bool mute);
    void onChannelMerged(const QDBusObjectPath &path);
    void onChannelSplitted(const QDBusObjectPath &path);

private:
    QString mObjPath;
    QString mPreviousState;
    bool mIncoming;
    bool mRequestedHangup;
    oFonoConnection *mConnection;
    QList<QDBusObjectPath> mCallChannels;
    Tp::BaseChannelPtr mBaseChannel;
    Tp::BaseChannelHoldInterfacePtr mHoldIface;
    Tp::BaseChannelConferenceInterfacePtr mConferenceIface;
    Tp::BaseChannelMergeableConferenceInterfacePtr mMergeableIface;
    Tp::BaseCallMuteInterfacePtr mMuteIface;
    BaseChannelAudioOutputsInterfacePtr mAudioOutputsIface;
    Tp::BaseChannelCallTypePtr mCallChannel;
    Tp::BaseCallContentDTMFInterfacePtr mDTMFIface;
    Tp::BaseCallContentPtr mCallContent;
    bool mDtmfLock;
    QStringList mDtmfPendingStrings;
};

#endif // OFONOCONFERENCECALLCHANNEL_H
