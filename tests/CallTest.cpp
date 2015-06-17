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
Q_DECLARE_METATYPE(Tp::ChannelPtr);
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
    void testCallIncomingPrivateNumber();
    void testCallIncomingUnknownNumber();
    void testNumberNormalization_data();
    void testNumberNormalization();
    void testCallOutgoing();
    void testCallHold();
    void testCallDTMF();
    void testCallMute();
    void testCallConference();

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
    qRegisterMetaType<Tp::Channel::GroupMemberChangeDetails>();
    qRegisterMetaType<Tp::ChannelPtr>();
    qRegisterMetaType<Tp::PendingOperation*>();
    qRegisterMetaType<QList<Tp::ContactPtr> >();
    TelepathyHelper::instance();

    QSignalSpy spy(TelepathyHelper::instance(), SIGNAL(accountReady()));
    QTRY_COMPARE(spy.count(), 1);

    OfonoMockController::instance()->SimManagerSetPresence(true);
    OfonoMockController::instance()->SimManagerSetPinRequired("none");
    OfonoMockController::instance()->ModemSetOnline();
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
    QTest::qWait(10000);
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

void CallTest::testCallIncomingPrivateNumber()
{
    QSignalSpy spyNewCallChannel(mHandler, SIGNAL(callChannelAvailable(Tp::CallChannelPtr)));
    QSignalSpy spyNewCallApprover(mApprover, SIGNAL(newCall()));
    QSignalSpy spyOfonoCallAdded(OfonoMockController::instance(), SIGNAL(CallAdded(QDBusObjectPath, QVariantMap)));
    OfonoMockController::instance()->VoiceCallManagerIncomingCall("withheld");
    QTRY_COMPARE(spyOfonoCallAdded.count(), 1);
    QTRY_COMPARE(spyNewCallApprover.count(), 1);

    mApprover->acceptCall();
    QTRY_COMPARE(spyNewCallChannel.count(), 1);

    Tp::CallChannelPtr channel = spyNewCallChannel.first().first().value<Tp::CallChannelPtr>();
    QVERIFY(channel);

    QCOMPARE(channel->initiatorContact()->id(), QString("x-ofono-private"));

    QDBusObjectPath path = spyOfonoCallAdded.first().first().value<QDBusObjectPath>();
    OfonoMockController::instance()->VoiceCallHangup(path.path());
    QTRY_COMPARE(channel->callState(), Tp::CallStateEnded);
}

void CallTest::testCallIncomingUnknownNumber()
{
    QSignalSpy spyNewCallChannel(mHandler, SIGNAL(callChannelAvailable(Tp::CallChannelPtr)));
    QSignalSpy spyNewCallApprover(mApprover, SIGNAL(newCall()));
    QSignalSpy spyOfonoCallAdded(OfonoMockController::instance(), SIGNAL(CallAdded(QDBusObjectPath, QVariantMap)));
    OfonoMockController::instance()->VoiceCallManagerIncomingCall("");
    QTRY_COMPARE(spyOfonoCallAdded.count(), 1);
    QTRY_COMPARE(spyNewCallApprover.count(), 1);

    mApprover->acceptCall();
    QTRY_COMPARE(spyNewCallChannel.count(), 1);

    Tp::CallChannelPtr channel = spyNewCallChannel.first().first().value<Tp::CallChannelPtr>();
    QVERIFY(channel);

    QCOMPARE(channel->initiatorContact()->id(), QString("x-ofono-unknown"));

    QDBusObjectPath path = spyOfonoCallAdded.first().first().value<QDBusObjectPath>();
    OfonoMockController::instance()->VoiceCallHangup(path.path());
    QTRY_COMPARE(channel->callState(), Tp::CallStateEnded);
}

void CallTest::testNumberNormalization_data()
{
    QTest::addColumn<QString>("number");
    QTest::addColumn<QString>("expectedNumber");

    QTest::newRow("simple number") << "12345678" << "12345678";
    QTest::newRow("number with dash") << "1234-5678" << "12345678"; 
    QTest::newRow("number with area code") << "(123)12345678" << "12312345678";
    QTest::newRow("number with slash") << "+421 2/123 456 78" << "+421212345678";
}

void CallTest::testNumberNormalization()
{
    QFETCH(QString, number);
    QFETCH(QString, expectedNumber);

    Tp::AccountPtr account = TelepathyHelper::instance()->account();
    QSignalSpy spy(this, SIGNAL(contactsReceived(QList<Tp::ContactPtr>)));

    connect(account->connection()->contactManager()->contactsForIdentifiers(QStringList() << number),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingContactsFinished(Tp::PendingOperation*)));

    QTRY_COMPARE(spy.count(), 1);

    QList<Tp::ContactPtr> contacts = spy.first().first().value<QList<Tp::ContactPtr> >();
    QCOMPARE(contacts.count(), 1);
    QCOMPARE(contacts.first()->id(), QString(expectedNumber));
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

            // test failed tones
            QSignalSpy spyOfonoTonesReceived(OfonoMockController::instance(), SIGNAL(TonesReceived(const QString&)));
            OfonoMockController::instance()->VoiceCallManagerFailNextDtmf();
            content->startDTMFTone((Tp::DTMFEvent)QString("1").toInt(&ok));
            OfonoMockController::instance()->VoiceCallManagerFailNextDtmf();
            content->startDTMFTone((Tp::DTMFEvent)QString("2").toInt(&ok));
            content->startDTMFTone((Tp::DTMFEvent)QString("3").toInt(&ok));
            QTRY_COMPARE(spyOfonoTonesReceived.count(), 2);
            QCOMPARE(spyOfonoTonesReceived.first().first().value<QString>(), QString("1"));
            spyOfonoTonesReceived.removeFirst();
            QCOMPARE(spyOfonoTonesReceived.first().first().value<QString>(), QString("23"));
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

void CallTest::testCallConference()
{
    // Request the contact to start chatting to
    Tp::AccountPtr account = TelepathyHelper::instance()->account();
    QSignalSpy spy(this, SIGNAL(contactsReceived(QList<Tp::ContactPtr>)));
    QSignalSpy spyOfonoCallAdded(OfonoMockController::instance(), SIGNAL(CallAdded(QDBusObjectPath, QVariantMap)));
    QSignalSpy spyCallChannel(mHandler, SIGNAL(callChannelAvailable(Tp::CallChannelPtr)));

    // Call #1
    connect(account->connection()->contactManager()->contactsForIdentifiers(QStringList() << "333"),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingContactsFinished(Tp::PendingOperation*)));

    QTRY_COMPARE(spy.count(), 1);

    QList<Tp::ContactPtr> contacts = spy.first().first().value<QList<Tp::ContactPtr> >();
    QCOMPARE(contacts.count(), 1);
    QCOMPARE(contacts.first()->id(), QString("333"));

    Q_FOREACH(Tp::ContactPtr contact, contacts) {
        account->ensureAudioCall(contact, "audio", QDateTime::currentDateTime(), TP_QT_IFACE_CLIENT + ".TpOfonoTestHandler");
    }
    QTRY_COMPARE(spyOfonoCallAdded.count(), 1);
    QTRY_COMPARE(spyCallChannel.count(), 1);
    QDBusObjectPath path1 = spyOfonoCallAdded.first().first().value<QDBusObjectPath>();

    Tp::CallChannelPtr channel1 = spyCallChannel.first().first().value<Tp::CallChannelPtr>();
    QVERIFY(channel1);

    OfonoMockController::instance()->VoiceCallSetAlerting(path1.path());
    QTRY_COMPARE(channel1->callState(), Tp::CallStateInitialised);

    OfonoMockController::instance()->VoiceCallAnswer(path1.path());
    QTRY_COMPARE(channel1->callState(), Tp::CallStateActive);

    spy.clear();
    spyOfonoCallAdded.clear();
    spyCallChannel.clear();

    // Call #2

    connect(account->connection()->contactManager()->contactsForIdentifiers(QStringList() << "444"),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingContactsFinished(Tp::PendingOperation*)));

    QTRY_COMPARE(spy.count(), 1);

    contacts = spy.first().first().value<QList<Tp::ContactPtr> >();
    QCOMPARE(contacts.count(), 1);
    QCOMPARE(contacts.first()->id(), QString("444"));

    Q_FOREACH(Tp::ContactPtr contact, contacts) {
        account->ensureAudioCall(contact, "audio", QDateTime::currentDateTime(), TP_QT_IFACE_CLIENT + ".TpOfonoTestHandler");
    }
    QTRY_COMPARE(spyOfonoCallAdded.count(), 1);
    QTRY_COMPARE(spyCallChannel.count(), 1);
    QDBusObjectPath path2 = spyOfonoCallAdded.first().first().value<QDBusObjectPath>();

    Tp::CallChannelPtr channel2 = spyCallChannel.first().first().value<Tp::CallChannelPtr>();
    QVERIFY(channel2);

    OfonoMockController::instance()->VoiceCallSetAlerting(path2.path());
    QTRY_COMPARE(channel2->callState(), Tp::CallStateInitialised);

    OfonoMockController::instance()->VoiceCallAnswer(path2.path());
    QTRY_COMPARE(channel2->callState(), Tp::CallStateActive);

    spy.clear();
    spyOfonoCallAdded.clear();
    spyCallChannel.clear();

    // create conference
    QList<Tp::ChannelPtr> calls;
    calls << channel1 << channel2;
    TelepathyHelper::instance()->account()->createConferenceCall(calls, QStringList(), QDateTime::currentDateTime(), TP_QT_IFACE_CLIENT + ".TpOfonoTestHandler");
    QTRY_COMPARE(spyCallChannel.count(), 1);

    Tp::CallChannelPtr conferenceChannel = spyCallChannel.first().first().value<Tp::CallChannelPtr>();
    QVERIFY(conferenceChannel);
    
    QSignalSpy spyConferenceChannelMerged(conferenceChannel.data(), SIGNAL(conferenceChannelMerged(const Tp::ChannelPtr &)));
    QSignalSpy spyConferenceChannelRemoved(conferenceChannel.data(), SIGNAL(conferenceChannelRemoved(const Tp::ChannelPtr &, const Tp::Channel::GroupMemberChangeDetails &)));

    spy.clear();
    spyOfonoCallAdded.clear();
    spyCallChannel.clear();

    // Call #3

    connect(account->connection()->contactManager()->contactsForIdentifiers(QStringList() << "555"),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingContactsFinished(Tp::PendingOperation*)));

    QTRY_COMPARE(spy.count(), 1);

    contacts = spy.first().first().value<QList<Tp::ContactPtr> >();
    QCOMPARE(contacts.count(), 1);
    QCOMPARE(contacts.first()->id(), QString("555"));

    Q_FOREACH(Tp::ContactPtr contact, contacts) {
        account->ensureAudioCall(contact, "audio", QDateTime::currentDateTime(), TP_QT_IFACE_CLIENT + ".TpOfonoTestHandler");
    }
    QTRY_COMPARE(spyOfonoCallAdded.count(), 1);
    QTRY_COMPARE(spyCallChannel.count(), 1);
    QDBusObjectPath path3 = spyOfonoCallAdded.first().first().value<QDBusObjectPath>();

    Tp::CallChannelPtr channel3 = spyCallChannel.first().first().value<Tp::CallChannelPtr>();
    QVERIFY(channel3);

    OfonoMockController::instance()->VoiceCallSetAlerting(path3.path());
    QTRY_COMPARE(channel3->callState(), Tp::CallStateInitialised);

    OfonoMockController::instance()->VoiceCallAnswer(path3.path());
    QTRY_COMPARE(channel3->callState(), Tp::CallStateActive);

    spy.clear();
    spyOfonoCallAdded.clear();
    spyCallChannel.clear();

    // merge channel 3 into the conference
    conferenceChannel->conferenceMergeChannel(channel3);
    QTRY_COMPARE(spyConferenceChannelMerged.count(), 1);

    // hangup first call so we end up with 2 calls in a conference
    OfonoMockController::instance()->VoiceCallHangup(path1.path());
    QTRY_COMPARE(channel1->callState(), Tp::CallStateEnded);
    QTRY_COMPARE(spyConferenceChannelRemoved.count(), 1);

    // split conference so we end up with 2 individual calls
    channel2->conferenceSplitChannel();
    QTRY_COMPARE(channel2->callState(), Tp::CallStateActive);

    // check if the conference was finished
    QTRY_COMPARE(conferenceChannel->callState(), Tp::CallStateEnded);

    OfonoMockController::instance()->VoiceCallHangup(path2.path());
    QTRY_COMPARE(channel2->callState(), Tp::CallStateEnded);

    OfonoMockController::instance()->VoiceCallHangup(path3.path());
    QTRY_COMPARE(channel3->callState(), Tp::CallStateEnded);
    

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
