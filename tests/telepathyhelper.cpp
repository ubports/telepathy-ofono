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

#include "telepathyhelper.h"
#include <TelepathyQt/AbstractClient>
#include <TelepathyQt/AccountSet>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingAccount>

TelepathyHelper::TelepathyHelper(QObject *parent)
    : QObject(parent),
      //mChannelObserver(0),
      mFirstTime(true),
      mConnected(false),
      mHandlerInterface(0)
{
    Tp::registerTypes();

    mAccountFeatures << Tp::Account::FeatureCore;
    mContactFeatures << Tp::Contact::FeatureAlias
                     << Tp::Contact::FeatureAvatarData
                     << Tp::Contact::FeatureAvatarToken
                     << Tp::Contact::FeatureCapabilities
                     << Tp::Contact::FeatureSimplePresence;
    mConnectionFeatures << Tp::Connection::FeatureCore
                        << Tp::Connection::FeatureSelfContact
                        << Tp::Connection::FeatureSimplePresence;

    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(QDBusConnection::sessionBus());
    channelFactory->addCommonFeatures(Tp::Channel::FeatureCore);

    mAccountManager = Tp::AccountManager::create(
            Tp::AccountFactory::create(QDBusConnection::sessionBus(), mAccountFeatures),
            Tp::ConnectionFactory::create(QDBusConnection::sessionBus(), mConnectionFeatures),
            channelFactory,
            Tp::ContactFactory::create(mContactFeatures));

    connect(mAccountManager->becomeReady(Tp::AccountManager::FeatureCore),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccountManagerReady(Tp::PendingOperation*)));

    mClientRegistrar = Tp::ClientRegistrar::create(mAccountManager);
}

TelepathyHelper::~TelepathyHelper()
{
}

TelepathyHelper *TelepathyHelper::instance()
{
    static TelepathyHelper* helper = new TelepathyHelper();
    return helper;
}

QString TelepathyHelper::accountId() const
{
    if (mAccount) {
        return mAccount->uniqueIdentifier();
    }
    return QString();
}

Tp::AccountPtr TelepathyHelper::account() const
{
    return mAccount;
}

/*
ChannelObserver *TelepathyHelper::channelObserver() const
{
    return mChannelObserver;
}
*/

bool TelepathyHelper::connected() const
{
    return mConnected;
}

/*
void TelepathyHelper::registerChannelObserver(const QString &observerName)
{
    QString name = observerName;

    if (name.isEmpty()) {
        name = "TelephonyPluginObserver";
    }

    if (mChannelObserver) {
        mChannelObserver->deleteLater();
    }

    mChannelObserver = new ChannelObserver(this);
    registerClient(mChannelObserver, name);

    // messages
    connect(mChannelObserver, SIGNAL(textChannelAvailable(Tp::TextChannelPtr)),
            ChatManager::instance(), SLOT(onTextChannelAvailable(Tp::TextChannelPtr)));

    // calls
    connect(mChannelObserver, SIGNAL(callChannelAvailable(Tp::CallChannelPtr)),
            CallManager::instance(), SLOT(onCallChannelAvailable(Tp::CallChannelPtr)));

    Q_EMIT channelObserverCreated(mChannelObserver);
}

void TelepathyHelper::unregisterChannelObserver()
{
    Tp::AbstractClientPtr clientPtr(mChannelObserver);
    if (clientPtr) {
        mClientRegistrar->unregisterClient(clientPtr);
    }
    mChannelObserver->deleteLater();
    mChannelObserver = NULL;
    Q_EMIT channelObserverUnregistered();
}
*/

QStringList TelepathyHelper::supportedProtocols() const
{
    QStringList protocols;
    protocols << "ufa"
              << "tel"
              << "ofono";
    return protocols;
}

void TelepathyHelper::initializeAccount()
{
    // watch for account state and connection changes
    connect(mAccount.data(),
            SIGNAL(stateChanged(bool)),
            SLOT(onAccountStateChanged(bool)));
    connect(mAccount.data(),
            SIGNAL(connectionChanged(const Tp::ConnectionPtr&)),
            SLOT(onAccountConnectionChanged(const Tp::ConnectionPtr&)));

    // and make sure it is enabled and connected
    if (!mAccount->isEnabled()) {
        ensureAccountEnabled();
    } else {
        ensureAccountConnected();
    }
}

void TelepathyHelper::ensureAccountEnabled()
{
    mAccount->setConnectsAutomatically(true);
    connect(mAccount->setEnabled(true),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccountEnabled(Tp::PendingOperation*)));
}

void TelepathyHelper::ensureAccountConnected()
{
    // if the account is not connected, request it to connect
    if (!mAccount->connection() || mAccount->connectionStatus() != Tp::ConnectionStatusConnected) {
        Tp::Presence presence(Tp::ConnectionPresenceTypeAvailable, "available", "online");
        mAccount->setRequestedPresence(presence);
    } else {
        watchSelfContactPresence();
    }

    if (mFirstTime) {
        Q_EMIT accountReady();
        mFirstTime = false;
    }
}

void TelepathyHelper::watchSelfContactPresence()
{
    if (mAccount.isNull() || mAccount->connection().isNull()) {
        return;
    }

    connect(mAccount->connection()->selfContact().data(),
            SIGNAL(presenceChanged(Tp::Presence)),
            SLOT(onPresenceChanged(Tp::Presence)));
    onPresenceChanged(mAccount->connection()->selfContact()->presence());
}

void TelepathyHelper::registerClient(Tp::AbstractClient *client, QString name)
{
    Tp::AbstractClientPtr clientPtr(client);
    bool succeeded = mClientRegistrar->registerClient(clientPtr, name);
    if (!succeeded) {
        name.append("%1");
        int count = 0;
        // limit the number of registered clients to 20, that should be a safe margin
        while (!succeeded && count < 20) {
            succeeded = mClientRegistrar->registerClient(clientPtr, name.arg(++count));
            if (succeeded) {
                name = name.arg(count);
            }
        }
    }

    if (succeeded) {
        QObject *object = dynamic_cast<QObject*>(client);
        if (object) {
            object->setProperty("clientName", TP_QT_IFACE_CLIENT + "." + name );
        }
    }
}

void TelepathyHelper::onAccountManagerReady(Tp::PendingOperation *op)
{
    Q_UNUSED(op)

    Tp::AccountSetPtr accountSet;
    // try to find an account of the one of supported protocols
    Q_FOREACH(const QString &protocol, supportedProtocols()) {
        accountSet = mAccountManager->accountsByProtocol(protocol);
        if (accountSet->accounts().count() > 0) {
            break;
        }
    }

    if (accountSet->accounts().count() == 0) {
        qCritical() << "No compatible telepathy account found!";
        return;
    }

    mAccount = accountSet->accounts()[0];

    // in case we have more than one account, the first one to show on the list is going to be used
    if (accountSet->accounts().count() > 1) {
        qWarning() << "There are more than just one account of type" << mAccount->protocolName();
    }

    Q_EMIT accountIdChanged();

    initializeAccount();
}

void TelepathyHelper::onAccountEnabled(Tp::PendingOperation *op)
{
    // we might need to do more stuff once the account is enabled, but making sure it is connected is a good start
    ensureAccountConnected();
}

void TelepathyHelper::onAccountStateChanged(bool enabled)
{
    if (!enabled) {
        ensureAccountEnabled();
    }
}

void TelepathyHelper::onAccountConnectionChanged(const Tp::ConnectionPtr &connection)
{
    if (connection.isNull()) {
        ensureAccountConnected();
    } else {
        watchSelfContactPresence();
    }
    Q_EMIT connectionChanged();
}

void TelepathyHelper::onPresenceChanged(const Tp::Presence &presence)
{
    mConnected = (presence.type() == Tp::ConnectionPresenceTypeAvailable);
    Q_EMIT connectedChanged();
}
