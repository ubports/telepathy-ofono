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

#include "ofonoconferencecallchannel.h"
#ifdef USE_PULSEAUDIO
#include "qpulseaudioengine.h"
#endif
#include "ofonocallchannel.h"


oFonoConferenceCallChannel::oFonoConferenceCallChannel(oFonoConnection *conn, QObject *parent):
    mRequestedHangup(false),
    mConnection(conn),
    mDtmfLock(false)
{

    Q_FOREACH(oFonoCallChannel *channel, mConnection->callChannels().values()) {
        if (channel->callState() == Tp::CallStateActive) {
            QDBusObjectPath path(channel->baseChannel()->objectPath());
            mCallChannels[path] = channel;
        }
    }

    Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(mConnection, TP_QT_IFACE_CHANNEL_TYPE_CALL, 0, Tp::HandleTypeNone);
    Tp::BaseChannelCallTypePtr callType = Tp::BaseChannelCallType::create(baseChannel.data(),
                                                                          true,
                                                                          Tp::StreamTransportTypeUnknown,
                                                                          true,
                                                                          false, "","");
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(callType));

    mHoldIface = Tp::BaseChannelHoldInterface::create();
    mHoldIface->setSetHoldStateCallback(Tp::memFun(this,&oFonoConferenceCallChannel::onHoldStateChanged));

    mMuteIface = Tp::BaseCallMuteInterface::create();
    mMuteIface->setSetMuteStateCallback(Tp::memFun(this,&oFonoConferenceCallChannel::onMuteStateChanged));

    mSpeakerIface = BaseChannelSpeakerInterface::create();
    mSpeakerIface->setTurnOnSpeakerCallback(Tp::memFun(this,&oFonoConferenceCallChannel::onTurnOnSpeaker));

    mConferenceIface = Tp::BaseChannelConferenceInterface::create(mCallChannels.keys());

    mMergeableIface = Tp::BaseChannelMergeableConferenceInterface::create();
    mMergeableIface->setMergeCallback(Tp::memFun(this,&oFonoConferenceCallChannel::onMerge));

    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mHoldIface));
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mMuteIface));
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mSpeakerIface));
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mConferenceIface));
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mMergeableIface));

    mBaseChannel = baseChannel;
    mCallChannel = Tp::BaseChannelCallTypePtr::dynamicCast(mBaseChannel->interface(TP_QT_IFACE_CHANNEL_TYPE_CALL));

    mCallChannel->setHangupCallback(Tp::memFun(this,&oFonoConferenceCallChannel::onHangup));

    Tp::CallStateReason reason;
    QVariantMap stateDetails;
    reason.actor =  0;
    reason.reason = Tp::CallStateChangeReasonUserRequested;
    reason.message = "";
    reason.DBusReason = "";

    mCallChannel->setCallState(Tp::CallStateActive, 0, reason, stateDetails);

    // init must be called after initialization, otherwise we will have no object path registered.
    QTimer::singleShot(0, this, SLOT(init()));
}

Tp::BaseChannelPtr oFonoConferenceCallChannel::baseChannel()
{
    return mBaseChannel;
}

void oFonoConferenceCallChannel::onMerge(const QDBusObjectPath &channel, Tp::DBusError *error)
{
    // on gsm we always merge all the existing calls
    mConnection->voiceCallManager()->createMultiparty();
}

void oFonoConferenceCallChannel::onChannelMerged(oFonoCallChannel *channel)
{
    QDBusObjectPath path(channel->baseChannel()->objectPath());
    if (!mCallChannels.keys().contains(path)) {
        mCallChannels[path] = channel;
        mConferenceIface->mergeChannel(path, 0, Tp::QualifiedPropertyValueMap());
    }
}

void oFonoConferenceCallChannel::onChannelSplitted(oFonoCallChannel *channel)
{
    QDBusObjectPath path(channel->baseChannel()->objectPath());
    if (mCallChannels.keys().contains(path)) {
        mCallChannels.remove(path);
        mConferenceIface->removeChannel(path, QVariantMap());
    }
    if (mCallChannels.size() == 1) {
        Tp::CallStateReason reason;
        QVariantMap stateDetails;
        reason.actor =  0;
        reason.reason = Tp::CallStateChangeReasonUserRequested;
        reason.message = "";
        reason.DBusReason = "";

        mCallChannel->setCallState(Tp::CallStateEnded, 0, reason, stateDetails);
        mBaseChannel->close();
    }
}

void oFonoConferenceCallChannel::onTurnOnSpeaker(bool active, Tp::DBusError *error)
{
    mConnection->setSpeakerMode(active);
}

void oFonoConferenceCallChannel::onHangup(uint reason, const QString &detailedReason, const QString &message, Tp::DBusError *error)
{
    // TODO: use the parameters sent by telepathy
    //mRequestedHangup = true;
    mConnection->voiceCallManager()->hangupMultiparty();
    //hangup();
}

void oFonoConferenceCallChannel::init()
{
    QVariantMap stateDetails;
    Tp::CallStateReason reason;

    mObjPath = mBaseChannel->objectPath();

    reason.actor =  0;
    reason.reason = Tp::CallStateChangeReasonProgressMade;
    reason.message = "";
    reason.DBusReason = "";

    mCallChannel->setCallState(Tp::CallStateActive, 0, reason, stateDetails);

    mDTMFIface = Tp::BaseCallContentDTMFInterface::create();

    mDTMFIface->setStartToneCallback(Tp::memFun(this,&oFonoConferenceCallChannel::onDTMFStartTone));
    mDTMFIface->setStopToneCallback(Tp::memFun(this,&oFonoConferenceCallChannel::onDTMFStopTone));

    QObject::connect(mBaseChannel.data(), SIGNAL(closed()), this, SLOT(deleteLater()));
    QObject::connect(mConnection->callVolume(), SIGNAL(mutedChanged(bool)), SLOT(onOfonoMuteChanged(bool)));
    QObject::connect(mConnection, SIGNAL(speakerModeChanged(bool)), mSpeakerIface.data(), SLOT(setSpeakerMode(bool)));
    QObject::connect(mConnection->voiceCallManager(), SIGNAL(sendTonesComplete(bool)), SLOT(onDtmfComplete(bool)));

    mSpeakerIface->setSpeakerMode(mConnection->speakerMode());
    QObject::connect(mConnection, SIGNAL(channelMerged(oFonoCallChannel*)), this, SLOT(onChannelMerged(oFonoCallChannel*)));
    QObject::connect(mConnection, SIGNAL(channelSplitted(oFonoCallChannel*)), this, SLOT(onChannelSplitted(oFonoCallChannel*)));
}

void oFonoConferenceCallChannel::onOfonoMuteChanged(bool mute)
{
    Tp::LocalMuteState state = mute ? Tp::LocalMuteStateMuted : Tp::LocalMuteStateUnmuted;
    mMuteIface->setMuteState(state);
}

void oFonoConferenceCallChannel::onHoldStateChanged(const Tp::LocalHoldState &state, const Tp::LocalHoldStateReason &reason, Tp::DBusError *error)
{
    mConnection->voiceCallManager()->swapCalls();
}

void oFonoConferenceCallChannel::onMuteStateChanged(const Tp::LocalMuteState &state, Tp::DBusError *error)
{
    if (state == Tp::LocalMuteStateMuted) {
        mConnection->callVolume()->setMuted(true);
#ifdef USE_PULSEAUDIO
        QPulseAudioEngine::instance()->setMicMute(true);
#endif
    } else if (state == Tp::LocalMuteStateUnmuted) {
        mConnection->callVolume()->setMuted(false);
#ifdef USE_PULSEAUDIO
        QPulseAudioEngine::instance()->setMicMute(false);
#endif
    }
}

void oFonoConferenceCallChannel::sendNextDtmf()
{
    if (mDtmfLock) {
        return;
    }
    if (!mDtmfPendingStrings.isEmpty()) {
        mDtmfLock = true;
        mConnection->voiceCallManager()->sendTones(mDtmfPendingStrings.front());
    }
}

void oFonoConferenceCallChannel::onDtmfComplete(bool success)
{
    mDtmfLock = false;
    if (success) {
        mDtmfPendingStrings.removeFirst();
       if (mDtmfPendingStrings.isEmpty()) {
           return;
       }
       sendNextDtmf();
    } else {
        QTimer::singleShot(1000, this, SLOT(sendNextDtmf()));
    }
}

void oFonoConferenceCallChannel::onDTMFStartTone(uchar event, Tp::DBusError *error)
{
    QString finalString;
    if (event == 10) {
        finalString = "*";
    } else if (event == 11) {
        finalString = "#";
    } else {
        finalString = QString::number(event);
    }

    qDebug() << "start tone" << finalString;
    // we can't append to the first item in the queue as it is being sent and 
    // we dont know yet if it will succeed or not.
    if (mDtmfPendingStrings.count() > 1) {
        mDtmfPendingStrings[1] += finalString;
    } else {
        mDtmfPendingStrings << finalString;
    }
    sendNextDtmf();
}

void oFonoConferenceCallChannel::onDTMFStopTone(Tp::DBusError *error)
{
}

oFonoConferenceCallChannel::~oFonoConferenceCallChannel()
{
    qDebug() << "conference call channel closed";
    // TODO - for some reason the object is not being removed
    mConnection->dbusConnection().unregisterObject(mObjPath, QDBusConnection::UnregisterTree);
}
