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
#include <TelepathyQt/CallChannel>
#include "telepathyhelper.h"
#include "ofonomockcontroller.h"
#include "handler.h"
#include "approvercall.h"

Q_DECLARE_METATYPE(Tp::CallChannelPtr);
Q_DECLARE_METATYPE(Tp::CallState);
Q_DECLARE_METATYPE(QList<Tp::ContactPtr>);

class CallTest : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void contactsReceived(QList<Tp::ContactPtr> contacts);

private Q_SLOTS:
    void initTestCase();
    void testCallReceived();
    void testCallSend();

    // helper slots
    void onPendingContactsFinished(Tp::PendingOperation*);

private:
    Approver *mApprover;
    Handler *mHandler;
};

void CallTest::initTestCase()
{
    qRegisterMetaType<Tp::Presence>();
    qRegisterMetaType<Tp::CallState>();
    qRegisterMetaType<Tp::CallChannelPtr>();
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

    mApprover = new Approver(this);
    TelepathyHelper::instance()->registerClient(mApprover, "TpOfonoTestApprover");
    QTRY_VERIFY(QDBusConnection::sessionBus().interface()->isServiceRegistered(TELEPHONY_SERVICE_APPROVER));

    // we need to wait in order to give telepathy time to notify about the approver and handler
    QTest::qWait(2000);
}

void CallTest::testCallReceived()
{
    QSignalSpy spyNewCallChannel(mHandler, SIGNAL(callChannelAvailable(Tp::CallChannelPtr)));
    QSignalSpy spyNewCallApprover(mApprover, SIGNAL(newCall()));
    OfonoMockController::instance()->VoiceCallManagerIncomingCall("123");
    QTRY_COMPARE(spyNewCallApprover.count(), 1);

    mApprover->acceptCall();
    QTRY_COMPARE(spyNewCallChannel.count(), 1);

    Tp::CallChannelPtr channel = spyNewCallChannel.first().first().value<Tp::CallChannelPtr>();
    QVERIFY(channel);
}

void CallTest::testCallSend()
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

    QSignalSpy spyOfonoCallAdded(OfonoMockController::instance(), SIGNAL(CallAdded(QDBusObjectPath, QVariantMap)));
    QSignalSpy spyCallChannel(mHandler, SIGNAL(callChannelAvailable(Tp::CallChannelPtr)));

    Q_FOREACH(Tp::ContactPtr contact, contacts) {
        account->ensureAudioCall(contact, "audio", QDateTime::currentDateTime(), TP_QT_IFACE_CLIENT + ".TpOfonoTestHandler");
    }
    QTRY_COMPARE(spyOfonoCallAdded.count(), 1);
    QTRY_COMPARE(spyCallChannel.count(), 1);
    QDBusObjectPath path = spyOfonoCallAdded.first().first().value<QDBusObjectPath>();

    Tp::CallChannelPtr channel = spyCallChannel.first().first().value<Tp::CallChannelPtr>();
    QVERIFY(channel);

    OfonoMockController::instance()->VoiceCallSetAlerting(path.path());
    QTRY_COMPARE(channel->callState(), Tp::CallStateInitialised);

    OfonoMockController::instance()->VoiceCallAnswer(path.path());
    QTRY_COMPARE(channel->callState(), Tp::CallStateActive);

    OfonoMockController::instance()->VoiceCallHangup(path.path());
    QTRY_COMPARE(channel->callState(), Tp::CallStateEnded);
}


void CallTest::onPendingContactsFinished(Tp::PendingOperation *op)
{
    Tp::PendingContacts *pc = qobject_cast<Tp::PendingContacts*>(op);
    if (!pc) {
        return;
    }

    Q_EMIT contactsReceived(pc->contacts());
}

QTEST_MAIN(CallTest)
#include "CallTest.moc"
