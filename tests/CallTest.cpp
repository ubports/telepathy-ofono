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
#include <TelepathyQt/CallChannel>
#include "telepathyhelper.h"
#include "ofonomockcontroller.h"
#include "handler.h"
#include "approvercall.h"

Q_DECLARE_METATYPE(Tp::CallChannelPtr);
Q_DECLARE_METATYPE(Tp::CallState);
Q_DECLARE_METATYPE(QList<Tp::ContactPtr>);

#define TELEPATHY_MUTE_IFACE "org.freedesktop.Telepathy.Call1.Interface.Mute"

class CallTest : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void contactsReceived(QList<Tp::ContactPtr> contacts);

private Q_SLOTS:
    void initTestCase();
    void testCallIncoming();
    void testCallOutgoing();
    void testCallHold();
    void testCallDTMF();
    void testCallMute();

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
    QTest::qWait(3000);
}

void CallTest::testCallIncoming()
{
    QSignalSpy spyNewCallChannel(mHandler, SIGNAL(callChannelAvailable(Tp::CallChannelPtr)));
    QSignalSpy spyNewCallApprover(mApprover, SIGNAL(newCall()));
    QSignalSpy spyOfonoCallAdded(OfonoMockController::instance(), SIGNAL(CallAdded(QDBusObjectPath, QVariantMap)));
    OfonoMockController::instance()->VoiceCallManagerIncomingCall("123");
    QTRY_COMPARE(spyOfonoCallAdded.count(), 1);
    QTRY_COMPARE(spyNewCallApprover.count(), 1);

    mApprover->acceptCall();
    QTRY_COMPARE(spyNewCallChannel.count(), 1);

    Tp::CallChannelPtr channel = spyNewCallChannel.first().first().value<Tp::CallChannelPtr>();
    QVERIFY(channel);

    QDBusObjectPath path = spyOfonoCallAdded.first().first().value<QDBusObjectPath>();
    OfonoMockController::instance()->VoiceCallHangup(path.path());
    QTRY_COMPARE(channel->callState(), Tp::CallStateEnded);
}

void CallTest::testCallOutgoing()
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

void CallTest::testCallHold()
{
    QSignalSpy spyNewCallChannel(mHandler, SIGNAL(callChannelAvailable(Tp::CallChannelPtr)));
    QSignalSpy spyNewCallApprover(mApprover, SIGNAL(newCall()));
    OfonoMockController::instance()->VoiceCallManagerIncomingCall("123");
    QTRY_COMPARE(spyNewCallApprover.count(), 1);

    mApprover->acceptCall();
    QTRY_COMPARE(spyNewCallChannel.count(), 1);

    Tp::CallChannelPtr channel = spyNewCallChannel.first().first().value<Tp::CallChannelPtr>();
    QVERIFY(channel);

    channel->accept();
    QTRY_COMPARE(channel->callState(), Tp::CallStateActive);

    channel->requestHold(true);
    QTRY_COMPARE(channel->localHoldState(), Tp::LocalHoldStateHeld);

    channel->requestHold(false);
    QTRY_COMPARE(channel->localHoldState(), Tp::LocalHoldStateUnheld);

    channel->hangup();
}

void CallTest::testCallDTMF()
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


    Q_FOREACH(const Tp::CallContentPtr &content, channel->contents()) {
        if (content->supportsDTMF()) {
            bool ok;
            QStringList keys;
            keys << "0" << "1" << "2" << "3" << "4" << "5" << "6" 
                        << "7" << "8" << "9" << "*" << "#";
            QStringListIterator keysIterator(keys);
            while (keysIterator.hasNext()) {
                QString key = keysIterator.next();
                Tp::DTMFEvent event = (Tp::DTMFEvent)key.toInt(&ok);
                if (!ok) {
                    if (!key.compare("*")) {
                        event = Tp::DTMFEventAsterisk;
                    } else if (!key.compare("#")) {
                        event = Tp::DTMFEventHash;
                    } else {
                        qWarning() << "Tone not recognized. DTMF failed";
                        return;
                    }
                }
                QSignalSpy spyOfonoTonesReceived(OfonoMockController::instance(), SIGNAL(TonesReceived(const QString&)));
                content->startDTMFTone(event);
                QTRY_COMPARE(spyOfonoTonesReceived.count(), 1);
                QCOMPARE(spyOfonoTonesReceived.first().first().value<QString>(), key);
            }
        }
    }

    OfonoMockController::instance()->VoiceCallHangup(path.path());
    QTRY_COMPARE(channel->callState(), Tp::CallStateEnded);
}

void CallTest::testCallMute()
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


    QDBusInterface muteInterface(channel->busName(), channel->objectPath(), TELEPATHY_MUTE_IFACE);
    QSignalSpy spyMuteChanged(OfonoMockController::instance(), SIGNAL(CallVolumeMuteChanged(bool)));
    muteInterface.call("RequestMuted", true);
    QTRY_COMPARE(spyMuteChanged.count(), 1);
    QTRY_COMPARE(spyMuteChanged.first().first().value<bool>(), true);
    spyMuteChanged.clear();

    muteInterface.call("RequestMuted", false);
    QTRY_COMPARE(spyMuteChanged.count(), 1);
    QTRY_COMPARE(spyMuteChanged.first().first().value<bool>(), false);

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
