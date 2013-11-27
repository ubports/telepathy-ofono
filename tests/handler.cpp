/*
 * Copyright (C) 2012-2013 Canonical, Ltd.
 *
 * Authors:
 *  Tiago Salem Herrmann <tiago.herrmann@canonical.com>
 *  Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
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

#include "handler.h"
#include "telepathyhelper.h"

#include <TelepathyQt/MethodInvocationContext>
#include <TelepathyQt/CallChannel>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/PendingReady>

Handler::Handler(QObject *parent)
    : QObject(parent), Tp::AbstractClientHandler(channelFilters())
{
}

bool Handler::bypassApproval() const
{
    return true;
}

void Handler::handleChannels(const Tp::MethodInvocationContextPtr<> &context,
                             const Tp::AccountPtr &account,
                             const Tp::ConnectionPtr &connection,
                             const QList<Tp::ChannelPtr> &channels,
                             const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
                             const QDateTime &userActionTime,
                             const Tp::AbstractClientHandler::HandlerInfo &handlerInfo)
{
    Q_UNUSED(account)
    Q_UNUSED(connection)
    Q_UNUSED(requestsSatisfied)
    Q_UNUSED(userActionTime)
    Q_UNUSED(handlerInfo)

    Q_FOREACH(const Tp::ChannelPtr channel, channels) {
        Tp::TextChannelPtr textChannel = Tp::TextChannelPtr::dynamicCast(channel);
        if (textChannel) {
            Tp::PendingReady *pr = textChannel->becomeReady(Tp::Features()
                                                         << Tp::TextChannel::FeatureCore
                                                         << Tp::TextChannel::FeatureChatState
                                                         << Tp::TextChannel::FeatureMessageCapabilities
                                                         << Tp::TextChannel::FeatureMessageQueue
                                                         << Tp::TextChannel::FeatureMessageSentSignal);

            connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onTextChannelReady(Tp::PendingOperation*)));

            mReadyRequests[pr] = textChannel;
            continue;
        }

        Tp::CallChannelPtr callChannel = Tp::CallChannelPtr::dynamicCast(channel);
        if (callChannel) {
            Tp::PendingReady *pr = callChannel->becomeReady(Tp::Features()
                                             << Tp::CallChannel::FeatureCore
                                             << Tp::CallChannel::FeatureCallState
                                             << Tp::CallChannel::FeatureContents);
            connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onCallChannelReady(Tp::PendingOperation*)));
            mReadyRequests[pr] = callChannel;
            continue;
        }

    }
    context->setFinished();
}

Tp::ChannelClassSpecList Handler::channelFilters()
{
    Tp::ChannelClassSpecList specList;
    specList << Tp::ChannelClassSpec::audioCall();
    specList << Tp::ChannelClassSpec::textChat();

    return specList;
}

void Handler::onTextChannelReady(Tp::PendingOperation *op)
{
    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);

    if (!pr) {
        qCritical() << "The pending object is not a Tp::PendingReady";
        return;
    }

    Tp::ChannelPtr channel = mReadyRequests[pr];
    Tp::TextChannelPtr textChannel = Tp::TextChannelPtr::dynamicCast(channel);

    if(!textChannel) {
        qCritical() << "The saved channel is not a Tp::TextChannel";
        return;
    }

    mReadyRequests.remove(pr);

    Q_EMIT textChannelAvailable(textChannel);
}

void Handler::onCallChannelReady(Tp::PendingOperation *op)
{
    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);

    if (!pr) {
        qCritical() << "The pending object is not a Tp::PendingReady";
        return;
    }

    Tp::ChannelPtr channel = mReadyRequests[pr];
    Tp::CallChannelPtr callChannel = Tp::CallChannelPtr::dynamicCast(channel);

    if(!callChannel) {
        qCritical() << "The saved channel is not a Tp::CallChannel";
        return;
    }

    mReadyRequests.remove(pr);

    Q_EMIT callChannelAvailable(callChannel);
}
