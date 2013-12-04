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

#include <QtCore/QObject>
#include <QtTest/QtTest>
#include <QVariant>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/ReceivedMessage>
#include "telepathyhelper.h"
#include "ofonomockcontroller.h"
#include "handler.h"
#include "approvertext.h"

Q_DECLARE_METATYPE(Tp::TextChannelPtr);
Q_DECLARE_METATYPE(QList<Tp::ContactPtr>);

class MessagesTest : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void contactsReceived(QList<Tp::ContactPtr> contacts);

private Q_SLOTS:
    void initTestCase();
    void testMessageReceived();
    void testMessageSend();

    // helper slots
    void onPendingContactsFinished(Tp::PendingOperation*);

private:
    Approver *mApprover;
    Handler *mHandler;
};

void MessagesTest::initTestCase()
{
    qRegisterMetaType<Tp::Presence>();
    qRegisterMetaType<Tp::TextChannelPtr>();
    qRegisterMetaType<Tp::PendingOperation*>();
    qRegisterMetaType<QList<Tp::ContactPtr> >();
    TelepathyHelper::instance();

    QSignalSpy spy(TelepathyHelper::instance(), SIGNAL(accountReady()));
    QTRY_COMPARE(spy.count(), 1);

    OfonoMockController::instance()->NetworkRegistrationSetStatus("registered");
    // the account should be connected
    QTRY_VERIFY(TelepathyHelper::instance()->connected());

    mHandler = new Handler(this);
    TelepathyHelper::instance()->registerClient(mHandler, "TpOfonoTestHandler");
    QTRY_VERIFY(mHandler->isRegistered());

    // register the approver
    mApprover = new Approver(this);
    TelepathyHelper::instance()->registerClient(mApprover, "TpOfonoTestApprover");
    // Tp-qt does not set registered status to approvers
    QTRY_VERIFY(QDBusConnection::sessionBus().interface()->isServiceRegistered(TELEPHONY_SERVICE_APPROVER));

    // we need to wait in order to give telepathy time to notify about the approver and handler
    QTest::qWait(2000); 
}

void MessagesTest::testMessageReceived()
{
    QSignalSpy spy(mHandler, SIGNAL(textChannelAvailable(Tp::TextChannelPtr)));
    OfonoMockController::instance()->MessageManagerSendMessage("123", "text");
    QTRY_COMPARE(spy.count(), 1);

    Tp::TextChannelPtr channel = spy.first().first().value<Tp::TextChannelPtr>();
    QVERIFY(channel);

    QCOMPARE(channel->messageQueue().count(), 1);

    Tp::ReceivedMessage message = channel->messageQueue().first();
    QCOMPARE(message.sender()->id(), QString("123"));
    QCOMPARE(message.text(), QString("text"));
}

void MessagesTest::testMessageSend()
{
    // Request the contact to start chatting to
    Tp::AccountPtr account = TelepathyHelper::instance()->account();
    QSignalSpy spy(this, SIGNAL(contactsReceived(QList<Tp::ContactPtr>)));

    connect(account->connection()->contactManager()->contactsForIdentifiers(QStringList() << "321"),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingContactsFinished(Tp::PendingOperation*)));

    QTRY_COMPARE(spy.count(), 1);

    QList<Tp::ContactPtr> contacts = spy.first().first().value<QList<Tp::ContactPtr> >();
    QCOMPARE(contacts.count(), 1);
    QCOMPARE(contacts.first()->id(), QString("321"));

    QSignalSpy spyTextChannel(mHandler, SIGNAL(textChannelAvailable(Tp::TextChannelPtr)));

    Q_FOREACH(Tp::ContactPtr contact, contacts) {
        account->ensureTextChat(contact, QDateTime::currentDateTime(), TP_QT_IFACE_CLIENT + ".TpOfonoTestHandler");
    }
    QTRY_COMPARE(spyTextChannel.count(), 1);

    Tp::TextChannelPtr channel = spyTextChannel.first().first().value<Tp::TextChannelPtr>();
    QVERIFY(channel);

    QSignalSpy spyOfonoMessageAdded(OfonoMockController::instance(), SIGNAL(MessageAdded(QDBusObjectPath, QVariantMap)));
    Tp::PendingSendMessage *message = channel->send("text");
    QTRY_COMPARE(spyOfonoMessageAdded.count(), 1);

    QDBusObjectPath path = spyOfonoMessageAdded.first().first().value<QDBusObjectPath>();

    // there is no way to use Tp::ReceivedMessage because it does not have a public default constructor, therefore 
    // we cannot use Q_DECLARE_METATYPE
    /* 
    QSignalSpy spyTpMessageReceived(channel.data(), SIGNAL(messageReceived(const Tp::ReceivedMessage&)));
    OfonoMockController::instance()->MessageMarkSent(path.path());

    // this is the acknowledge message
    QTRY_COMPARE(spyTpMessageReceived.count(), 1);
    */
}


void MessagesTest::onPendingContactsFinished(Tp::PendingOperation *op)
{
    Tp::PendingContacts *pc = qobject_cast<Tp::PendingContacts*>(op);
    if (!pc) {
        return;
    }

    Q_EMIT contactsReceived(pc->contacts());
}

QTEST_MAIN(MessagesTest)
#include "MessagesTest.moc"
