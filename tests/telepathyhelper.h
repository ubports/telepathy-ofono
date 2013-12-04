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
 * Authors: 
 *  Tiago Salem Herrmann <tiago.herrmann@canonical.com>
 *  Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
 */

#ifndef TELEPATHYHELPER_H
#define TELEPATHYHELPER_H

#include <QObject>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/Contact>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/Types>
//#include "channelobserver.h"

#define CANONICAL_TELEPHONY_VOICEMAIL_IFACE "com.canonical.Telephony.Voicemail"
#define CANONICAL_TELEPHONY_SPEAKER_IFACE "com.canonical.Telephony.Speaker"

class TelepathyHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString accountId READ accountId NOTIFY accountIdChanged)

public:
    ~TelepathyHelper();

    static TelepathyHelper *instance();
    Tp::AccountPtr account() const;
    //ChannelObserver *channelObserver() const;

    bool connected() const;
    QString accountId() const;

    void registerClient(Tp::AbstractClient *client, QString name);

Q_SIGNALS:
    //void channelObserverCreated(ChannelObserver *observer);
    //void channelObserverUnregistered();
    void accountReady();
    void connectionChanged();
    void connectedChanged();
    void accountIdChanged();

public Q_SLOTS:
    //Q_INVOKABLE void registerChannelObserver(const QString &observerName = QString::null);
    //Q_INVOKABLE void unregisterChannelObserver();

protected:
    QStringList supportedProtocols() const;
    void initializeAccount();
    void ensureAccountEnabled();
    void ensureAccountConnected();
    void watchSelfContactPresence();

private Q_SLOTS:
    void onAccountManagerReady(Tp::PendingOperation *op);
    void onAccountEnabled(Tp::PendingOperation *op);
    void onAccountStateChanged(bool enabled);
    void onAccountConnectionChanged(const Tp::ConnectionPtr &connection);
    void onPresenceChanged(const Tp::Presence &presence);

private:
    explicit TelepathyHelper(QObject *parent = 0);
    Tp::AccountManagerPtr mAccountManager;
    Tp::Features mAccountManagerFeatures;
    Tp::Features mAccountFeatures;
    Tp::Features mContactFeatures;
    Tp::Features mConnectionFeatures;
    Tp::ClientRegistrarPtr mClientRegistrar;
    Tp::AccountPtr mAccount;
    //ChannelObserver *mChannelObserver;
    bool mFirstTime;
    bool mConnected;
    QDBusInterface *mHandlerInterface;
};

#endif // TELEPATHYHELPER_H
