/*
 * Copyright (C) 2012 Canonical, Ltd.
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

#include "approvercall.h"
#include <QDebug>

#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/CallChannel>

Approver::Approver(QObject* parent)
: QObject(parent), Tp::AbstractClientApprover(channelFilters())
{
}

Approver::~Approver()
{
}

Tp::ChannelClassSpecList Approver::channelFilters() const
{
    Tp::ChannelClassSpecList specList;
    specList << Tp::ChannelClassSpec::audioCall();

    return specList;
}

void Approver::addDispatchOperation(const Tp::MethodInvocationContextPtr<> &context,
                                        const Tp::ChannelDispatchOperationPtr &dispatchOperation)
{
    bool willHandle = false;
    QList<Tp::ChannelPtr> channels = dispatchOperation->channels();
    Q_FOREACH (Tp::ChannelPtr channel, channels) {
        Tp::CallChannelPtr callChannel = Tp::CallChannelPtr::dynamicCast(channel);
        if (!callChannel.isNull()) {
            Tp::PendingReady *pr = callChannel->becomeReady(Tp::Features()
                                  << Tp::CallChannel::FeatureCore
                                  << Tp::CallChannel::FeatureCallState);
            mChannels[pr] = callChannel;

            connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onChannelReady(Tp::PendingOperation*)));
            callChannel->setProperty("accountId", QVariant(dispatchOperation->account()->uniqueIdentifier()));
            willHandle = true;
            continue;
        }
    }


    if (willHandle) {
        mDispatchOps.append(dispatchOperation);
    }

    context->setFinished();

    Q_EMIT newCall();
}

void Approver::acceptCall()
{
    Q_FOREACH (Tp::ChannelDispatchOperationPtr dispatchOperation, mDispatchOps) {
        QList<Tp::ChannelPtr> channels = dispatchOperation->channels();
        Q_FOREACH (Tp::ChannelPtr channel, channels) {
            if (dispatchOperation->possibleHandlers().contains(TELEPHONY_SERVICE_HANDLER)) {
                dispatchOperation->handleWith(TELEPHONY_SERVICE_HANDLER);
                mDispatchOps.removeAll(dispatchOperation);
            }
        }
    }
}

void Approver::rejectCall()
{
    Q_FOREACH (Tp::ChannelDispatchOperationPtr dispatchOperation, mDispatchOps) {
        QList<Tp::ChannelPtr> channels = dispatchOperation->channels();
        Q_FOREACH (Tp::ChannelPtr channel, channels) {
            if (dispatchOperation->possibleHandlers().contains(TELEPHONY_SERVICE_HANDLER)) {
                Tp::PendingOperation *claimop = dispatchOperation->claim();
                mChannels[claimop] = dispatchOperation->channels().first();
                connect(claimop, SIGNAL(finished(Tp::PendingOperation*)),
                    this, SLOT(onClaimFinished(Tp::PendingOperation*)));
            }
        }
    }
}

void Approver::onClaimFinished(Tp::PendingOperation* op)
{
    if(!op || op->isError()) {
        return;
    }

    Tp::CallChannelPtr callChannel = Tp::CallChannelPtr::dynamicCast(mChannels[op]);
    if (callChannel) {
        Tp::PendingOperation *hangupop = callChannel->hangup(Tp::CallStateChangeReasonUserRequested, TP_QT_ERROR_REJECTED, QString());
        mChannels[hangupop] = callChannel;
        connect(hangupop, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onHangupFinished(Tp::PendingOperation*)));
    }
}

void Approver::onHangupFinished(Tp::PendingOperation* op)
{
    if(!op || op->isError()) {
        return;
    }
    mDispatchOps.removeAll(dispatchOperation(op));
    mChannels.remove(op);
}

Tp::ChannelDispatchOperationPtr Approver::dispatchOperation(Tp::PendingOperation *op)
{
    Tp::ChannelPtr channel = mChannels[op];
    QString accountId = channel->property("accountId").toString();
    Q_FOREACH (Tp::ChannelDispatchOperationPtr dispatchOperation, mDispatchOps) {
        if (dispatchOperation->account()->uniqueIdentifier() == accountId) {
            return dispatchOperation;
        }
    }
    return Tp::ChannelDispatchOperationPtr();
}

