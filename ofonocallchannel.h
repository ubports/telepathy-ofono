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

#ifndef OFONOCALLCHANNEL_H
#define OFONOCALLCHANNEL_H

#include <QObject>

#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/BaseCall>
#include <TelepathyQt/Types>

#include <ofono-qt/ofonovoicecall.h>

#include "connection.h"
#include "speakeriface.h"

class oFonoConnection;

class oFonoCallChannel : public OfonoVoiceCall
{
    Q_OBJECT
public:
    oFonoCallChannel(oFonoConnection *conn, QString phoneNumber, uint targetHandle, QString voiceObj, QObject *parent = 0);
    ~oFonoCallChannel();
    Tp::BaseChannelPtr baseChannel();

    void onHangup(uint reason, const QString &detailedReason, const QString &message, Tp::DBusError* error);
    void onAccept(Tp::DBusError*);
    void onMuteStateChanged(const Tp::LocalMuteState &state, Tp::DBusError *error);
    void onHoldStateChanged(const Tp::LocalHoldState &state, const Tp::LocalHoldStateReason &reason, Tp::DBusError *error);
    void onDTMFStartTone(uchar event, Tp::DBusError *error);
    void onDTMFStopTone(Tp::DBusError *error);
    void onTurnOnSpeaker(bool active, Tp::DBusError *error);

private Q_SLOTS:
    void onOfonoCallStateChanged(const QString &state);
    void init();


    void onOfonoMuteChanged(bool mute);

private:
    QString mObjPath;
    QString mPreviousState;
    bool mIncoming;
    bool mRequestedHangup;
    Tp::BaseChannelPtr mBaseChannel;
    QString mPhoneNumber;
    oFonoConnection *mConnection;
    uint mTargetHandle;
    Tp::BaseChannelHoldInterfacePtr mHoldIface;
    Tp::BaseCallMuteInterfacePtr mMuteIface;
    BaseChannelSpeakerInterfacePtr mSpeakerIface;
    Tp::BaseChannelCallTypePtr mCallChannel;
    Tp::BaseCallContentDTMFInterfacePtr mDTMFIface;
    Tp::BaseCallContentPtr mCallContent;

};

#endif // OFONOCALLCHANNEL_H
