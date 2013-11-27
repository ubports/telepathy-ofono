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
#include "telepathyhelper.h"
#include "ofonomockcontroller.h"

class ConnectionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testConnected();
    void testModemStatus();
};

void ConnectionTest::initTestCase()
{
    TelepathyHelper::instance();
    QSignalSpy spy(TelepathyHelper::instance(),
                   SIGNAL(accountReady()));
    QTRY_COMPARE(spy.count(), 1);

    OfonoMockController::instance()->NetworkRegistrationSetStatus("registered");

    qRegisterMetaType<Tp::Presence>();
}

void ConnectionTest::testConnected()
{
    // the account should be connected automatically
    QTRY_VERIFY(TelepathyHelper::instance()->connected());
}

void ConnectionTest::testModemStatus()
{
    Tp::ContactPtr selfContact = TelepathyHelper::instance()->account()->connection()->selfContact();
    QSignalSpy signalSpy(selfContact.data(), SIGNAL(presenceChanged(Tp::Presence)));

    // set the status as unregistered
    OfonoMockController::instance()->NetworkRegistrationSetStatus("unregistered");
    QTRY_COMPARE(signalSpy.count(), 1);
    Tp::Presence presence = signalSpy.first().first().value<Tp::Presence>();
    QCOMPARE(presence.type(), Tp::ConnectionPresenceTypeOffline);
    signalSpy.clear();

    // now set the modem as registered to the network again to see if it works
    OfonoMockController::instance()->NetworkRegistrationSetStatus("registered");
    QTRY_COMPARE(signalSpy.count(), 1);
    presence = signalSpy.first().first().value<Tp::Presence>();
    QCOMPARE(presence.type(), Tp::ConnectionPresenceTypeAvailable);
    signalSpy.clear();

    // searching should be reported as offline
    OfonoMockController::instance()->NetworkRegistrationSetStatus("searching");
    QTRY_COMPARE(signalSpy.count(), 1);
    presence = signalSpy.first().first().value<Tp::Presence>();
    QCOMPARE(presence.type(), Tp::ConnectionPresenceTypeOffline);
    signalSpy.clear();

    // denied should be reported as offline (set registered first to force the signal to be emitted)
    OfonoMockController::instance()->NetworkRegistrationSetStatus("registered");
    QTRY_COMPARE(signalSpy.count(), 1);
    signalSpy.clear();
    OfonoMockController::instance()->NetworkRegistrationSetStatus("denied");
    QTRY_COMPARE(signalSpy.count(), 1);
    presence = signalSpy.first().first().value<Tp::Presence>();
    QCOMPARE(presence.type(), Tp::ConnectionPresenceTypeOffline);
    signalSpy.clear();

    // unknown should be reported as offline (set registered first to force the signal to be emitted)
    OfonoMockController::instance()->NetworkRegistrationSetStatus("registered");
    QTRY_COMPARE(signalSpy.count(), 1);
    signalSpy.clear();
    OfonoMockController::instance()->NetworkRegistrationSetStatus("unknown");
    QTRY_COMPARE(signalSpy.count(), 1);
    presence = signalSpy.first().first().value<Tp::Presence>();
    QCOMPARE(presence.type(), Tp::ConnectionPresenceTypeOffline);
    signalSpy.clear();

    // roaming should be reported as available
    OfonoMockController::instance()->NetworkRegistrationSetStatus("roaming");
    QTRY_COMPARE(signalSpy.count(), 1);
    presence = signalSpy.first().first().value<Tp::Presence>();
    QCOMPARE(presence.type(), Tp::ConnectionPresenceTypeAvailable);
}

QTEST_MAIN(ConnectionTest)
#include "ConnectionTest.moc"
