/*
 * Copyright (C) 2013 Canonical, Ltd.
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

Q_DECLARE_METATYPE(Tp::TextChannelPtr);

class MessagesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testMessageReceived();
    void testMessageSend();
private:
    Handler *mHandler;
};

void MessagesTest::initTestCase()
{
    qRegisterMetaType<Tp::Presence>();
    qRegisterMetaType<Tp::TextChannelPtr>();
    qRegisterMetaType<Tp::PendingOperation*>();
    TelepathyHelper::instance();

    QSignalSpy spy(TelepathyHelper::instance(), SIGNAL(accountReady()));
    QTRY_COMPARE(spy.count(), 1);

    OfonoMockController::instance()->NetworkRegistrationSetStatus("registered");

    mHandler = new Handler();
    TelepathyHelper::instance()->registerClient(mHandler, "TpOfonoTestHandler");
    QTRY_VERIFY(mHandler->isRegistered());

    // the account should be connected
    QTRY_VERIFY(TelepathyHelper::instance()->connected());

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
    QSignalSpy spy(account->connection()->contactManager()->contactsForIdentifiers(QStringList() << "321"),
            SIGNAL(finished(Tp::PendingOperation*)));

    QTRY_COMPARE(spy.count(), 1);
    
   //qDebug() << spy;
//    Tp::PendingContacts *pc = qobject_cast<Tp::PendingContacts*>(spy.first().first().value<Tp::PendingOperation*>());
//    QVERIFY(pc);

}

QTEST_MAIN(MessagesTest)
#include "MessagesTest.moc"
