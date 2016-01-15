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

#include "ofonocallchannel.h"


oFonoCallChannel::oFonoCallChannel(oFonoConnection *conn, QString phoneNumber, uint targetHandle, QString voiceObj, QObject *parent):
    OfonoVoiceCall(voiceObj),
    mIncoming(false),
    mRequestedHangup(false),
    mConnection(conn),
    mPhoneNumber(phoneNumber),
    mTargetHandle(targetHandle),
    mDtmfLock(false),
    mMultiparty(false)
    
{
    Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(mConnection, TP_QT_IFACE_CHANNEL_TYPE_CALL, Tp::HandleTypeContact, targetHandle);
    Tp::BaseChannelCallTypePtr callType = Tp::BaseChannelCallType::create(baseChannel.data(),
                                                                          true,
                                                                          Tp::StreamTransportTypeUnknown,
                                                                          true,
                                                                          false, "","");
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(callType));

    mHoldIface = Tp::BaseChannelHoldInterface::create();
    mHoldIface->setSetHoldStateCallback(Tp::memFun(this,&oFonoCallChannel::onHoldStateChanged));

    mMuteIface = Tp::BaseCallMuteInterface::create();
    mMuteIface->setSetMuteStateCallback(Tp::memFun(this,&oFonoCallChannel::onMuteStateChanged));

    mSplittableIface = Tp::BaseChannelSplittableInterface::create();
    mSplittableIface->setSplitCallback(Tp::memFun(this,&oFonoCallChannel::onSplit));

    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mHoldIface));
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mMuteIface));
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mSplittableIface));

    mBaseChannel = baseChannel;
    mCallChannel = Tp::BaseChannelCallTypePtr::dynamicCast(mBaseChannel->interface(TP_QT_IFACE_CHANNEL_TYPE_CALL));

    mCallChannel->setHangupCallback(Tp::memFun(this,&oFonoCallChannel::onHangup));
    mCallChannel->setAcceptCallback(Tp::memFun(this,&oFonoCallChannel::onAccept));

    QObject::connect(this, SIGNAL(hangupComplete(bool)), this, SLOT(onHangupComplete(bool)));
    QObject::connect(this, SIGNAL(answerComplete(bool)), this, SLOT(onAnswerComplete(bool)));
    // init must be called after initialization, otherwise we will have no object path registered.
    QTimer::singleShot(0, this, SLOT(init()));
}

Tp::CallState oFonoCallChannel::callState()
{
    return (Tp::CallState)mCallChannel->callState();
}

void oFonoCallChannel::onSplit(Tp::DBusError *error)
{
    mConnection->voiceCallManager()->privateChat(path());
}

void oFonoCallChannel::onHangupComplete(bool status)
{
    if (!status) {
        QTimer::singleShot(2000, this, SLOT(hangup()));
    }
}

void oFonoCallChannel::onAnswerComplete(bool status)
{
    if (!status) {
        QTimer::singleShot(1000, this, SLOT(answer()));
    }
}

void oFonoCallChannel::onHangup(uint reason, const QString &detailedReason, const QString &message, Tp::DBusError *error)
{
    // TODO: use the parameters sent by telepathy
    mRequestedHangup = true;
    hangup();
}

void oFonoCallChannel::onAccept(Tp::DBusError*)
{
    QVariantMap stateDetails;
    Tp::CallStateReason reason;
    reason.actor =  0;
    reason.reason = Tp::CallStateChangeReasonUserRequested;
    reason.message = "";
    reason.DBusReason = "";
    mCallChannel->setCallState(Tp::CallStateAccepted, 0, reason, stateDetails);

    if (this->state() == "waiting") {
        mConnection->voiceCallManager()->holdAndAnswer();
    } else {
        answer();
    }
}

void oFonoCallChannel::init()
{
    mIncoming = this->state() == "incoming" || this->state() == "waiting";
    mMultiparty = this->multiparty();
    mPreviousState = this->state();
    mObjPath = mBaseChannel->objectPath();

    Tp::CallMemberMap memberFlags;
    Tp::HandleIdentifierMap identifiers;
    QVariantMap stateDetails;
    Tp::CallStateReason reason;

    identifiers[mTargetHandle] = mPhoneNumber;
    reason.actor =  0;
    reason.reason = Tp::CallStateChangeReasonProgressMade;
    reason.message = "";
    reason.DBusReason = "";
    if (mIncoming) {
        memberFlags[mTargetHandle] = 0;
    } else {
        memberFlags[mTargetHandle] = Tp::CallMemberFlagRinging;
    }

    mCallChannel->setCallState(Tp::CallStateInitialising, 0, reason, stateDetails);

    mCallContent = Tp::BaseCallContent::create(baseChannel()->dbusConnection(), baseChannel().data(), "audio", Tp::MediaStreamTypeAudio, Tp::MediaStreamDirectionNone);

    mDTMFIface = Tp::BaseCallContentDTMFInterface::create();
    mCallContent->plugInterface(Tp::AbstractCallContentInterfacePtr::dynamicCast(mDTMFIface));
    mCallChannel->addContent(mCallContent);

    mDTMFIface->setStartToneCallback(Tp::memFun(this,&oFonoCallChannel::onDTMFStartTone));
    mDTMFIface->setStopToneCallback(Tp::memFun(this,&oFonoCallChannel::onDTMFStopTone));

    mCallChannel->setMembersFlags(memberFlags, identifiers, Tp::UIntList(), reason);

    mCallChannel->setCallState(Tp::CallStateInitialised, 0, reason, stateDetails);
    QObject::connect(mBaseChannel.data(), SIGNAL(closed()), this, SLOT(deleteLater()));
    QObject::connect(mConnection->callVolume(), SIGNAL(mutedChanged(bool)), SLOT(onOfonoMuteChanged(bool)));
    QObject::connect(this, SIGNAL(stateChanged(QString)), SLOT(onOfonoCallStateChanged(QString)));
    QObject::connect(mConnection->voiceCallManager(), SIGNAL(sendTonesComplete(bool)), SLOT(onDtmfComplete(bool)));
    QObject::connect(this, SIGNAL(multipartyChanged(bool)), this, SLOT(onMultipartyChanged(bool)));
    
    QObject::connect(this, SIGNAL(disconnectReason(const QString &)), this, SLOT(onDisconnectReason(const QString &)));
}

void oFonoCallChannel::onDisconnectReason(const QString &reason) {
    mRequestedHangup = reason == "local";
}

void oFonoCallChannel::onMultipartyChanged(bool multiparty)
{
    // if previous state was multparty then split
    if (multiparty) {
        Q_EMIT merged();
    } else {
        Q_EMIT splitted();
    }
    mMultiparty = multiparty;
}

void oFonoCallChannel::onOfonoMuteChanged(bool mute)
{
    Tp::LocalMuteState state = mute ? Tp::LocalMuteStateMuted : Tp::LocalMuteStateUnmuted;
    mMuteIface->setMuteState(state);
}

void oFonoCallChannel::onHoldStateChanged(const Tp::LocalHoldState &state, const Tp::LocalHoldStateReason &reason, Tp::DBusError *error)
{
    if (state == Tp::LocalHoldStateHeld && this->state() == "active") {
        QObject::connect(mConnection->voiceCallManager(), SIGNAL(swapCallsComplete(bool)), this, SLOT(onSwapCallsComplete(bool)));
        mConnection->voiceCallManager()->swapCalls();
        mHoldIface->setHoldState(Tp::LocalHoldStatePendingHold, Tp::LocalHoldStateReasonRequested);
    } else if (state == Tp::LocalHoldStateUnheld && this->state() == "held") {
        QObject::connect(mConnection->voiceCallManager(), SIGNAL(swapCallsComplete(bool)), this, SLOT(onSwapCallsComplete(bool)));
        mConnection->voiceCallManager()->swapCalls();
        mHoldIface->setHoldState(Tp::LocalHoldStatePendingUnhold, Tp::LocalHoldStateReasonRequested);
    }
}

void oFonoCallChannel::onSwapCallsComplete(bool success)
{
    if (!success && errorName() == "org.ofono.Error.InProgress") {
        QTimer::singleShot(2000, mConnection->voiceCallManager(), SLOT(swapCalls()));
        return;
    }
    Tp::LocalHoldState holdState = this->state() == "active" ? Tp::LocalHoldStateUnheld : Tp::LocalHoldStateHeld;
    Tp::LocalHoldStateReason reason = success ? Tp::LocalHoldStateReasonRequested : Tp::LocalHoldStateReasonResourceNotAvailable;
    QObject::disconnect(mConnection->voiceCallManager(), SIGNAL(swapCallsComplete(bool)), this, SLOT(onSwapCallsComplete(bool)));
    mHoldIface->setHoldState(holdState, reason);
}

void oFonoCallChannel::onMuteStateChanged(const Tp::LocalMuteState &state, Tp::DBusError *error)
{
    if (state == Tp::LocalMuteStateMuted) {
        mConnection->callVolume()->setMuted(true);
    } else if (state == Tp::LocalMuteStateUnmuted) {
        mConnection->callVolume()->setMuted(false);
    }
}

void oFonoCallChannel::sendNextDtmf()
{
    if (mDtmfLock) {
        return;
    }
    if (!mDtmfPendingStrings.isEmpty()) {
        mDtmfLock = true;
        mConnection->voiceCallManager()->sendTones(mDtmfPendingStrings.front());
    }
}

void oFonoCallChannel::onDtmfComplete(bool success)
{
    // this might be a response for another channel
    if (mCallChannel->callState() != Tp::CallStateActive) {
        return;
    }
    mDtmfLock = false;
    if (success) {
       if (mDtmfPendingStrings.count() > 0) {
           mDtmfPendingStrings.removeFirst();
       }
       if (mDtmfPendingStrings.isEmpty()) {
           return;
       }
       sendNextDtmf();
    } else {
        QTimer::singleShot(1000, this, SLOT(sendNextDtmf()));
    }
}

void oFonoCallChannel::onDTMFStartTone(uchar event, Tp::DBusError *error)
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

void oFonoCallChannel::onDTMFStopTone(Tp::DBusError *error)
{
}

oFonoCallChannel::~oFonoCallChannel()
{
    qDebug() << "call channel closed";
    // TODO - for some reason the object is not being removed
    mConnection->dbusConnection().unregisterObject(mObjPath, QDBusConnection::UnregisterTree);
}

Tp::BaseChannelPtr oFonoCallChannel::baseChannel()
{
    return mBaseChannel;
}

void oFonoCallChannel::onOfonoCallStateChanged(const QString &state)
{
    Tp::CallStateReason reason;
    QVariantMap stateDetails;
    reason.actor =  0;
    reason.reason = Tp::CallStateChangeReasonUserRequested;
    reason.message = "";
    reason.DBusReason = "";
    // we invalidate the pending dtmf strings if the call status is changed
    mDtmfPendingStrings.clear();
    mDtmfLock = false;
    if (state == "disconnected") {
        qDebug() << "disconnected";
        if (mIncoming && (mPreviousState == "incoming" || mPreviousState == "waiting") && !mRequestedHangup) {
            reason.reason = Tp::CallStateChangeReasonNoAnswer;
        }
        mCallChannel->setCallState(Tp::CallStateEnded, 0, reason, stateDetails);
        Q_EMIT closed();
        mBaseChannel->close();
    } else if (state == "active") {
        qDebug() << "active";
        mHoldIface->setHoldState(Tp::LocalHoldStateUnheld, Tp::LocalHoldStateReasonNone);
        if (mMultiparty) {
            Q_EMIT multipartyCallActive();
        }
        if (mPreviousState == "dialing" || mPreviousState == "alerting" || 
                mPreviousState == "incoming") {
            mConnection->callVolume()->setMuted(false);
            mCallChannel->setCallState(Tp::CallStateAccepted, 0, reason, stateDetails);
        }
        mCallChannel->setCallState(Tp::CallStateActive, 0, reason, stateDetails);
    } else if (state == "held") {
        mHoldIface->setHoldState(Tp::LocalHoldStateHeld, Tp::LocalHoldStateReasonNone);
        if (mMultiparty) {
            Q_EMIT multipartyCallHeld();
        }
        qDebug() << "held";
    } else if (state == "dialing") {
        qDebug() << "dialing";
    } else if (state == "alerting") {
        qDebug() << "alerting";
    } else if (state == "incoming") {
        qDebug() << "incoming";
    } else if (state == "waiting") {
        qDebug() << "waiting";
    }
    mPreviousState = state;
}
