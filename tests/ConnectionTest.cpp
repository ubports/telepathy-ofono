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
    void testUnregisterModem();
};

void ConnectionTest::initTestCase()
{
    TelepathyHelper::instance();
    QSignalSpy spy(TelepathyHelper::instance(),
                   SIGNAL(accountReady()));
    QTRY_COMPARE(spy.count(), 1);

    OfonoMockController::instance()->setNetworkRegistrationStatus("registered");

    qRegisterMetaType<Tp::Presence>();
}

void ConnectionTest::testConnected()
{
    // the account should be connected automatically
    QTRY_VERIFY(TelepathyHelper::instance()->connected());
}

void ConnectionTest::testUnregisterModem()
{
    Tp::ContactPtr selfContact = TelepathyHelper::instance()->account()->connection()->selfContact();
    QSignalSpy signalSpy(selfContact.data(), SIGNAL(presenceChanged(Tp::Presence)));

    // set the status as unregistered
    OfonoMockController::instance()->setNetworkRegistrationStatus("unknown");
    QTRY_COMPARE(signalSpy.count(), 1);
    Tp::Presence presence = signalSpy.first().first().value<Tp::Presence>();
    QCOMPARE(presence.type(), Tp::ConnectionPresenceTypeOffline);

    // now set the modem as registered to the network again to see if it works
    signalSpy.clear();
    OfonoMockController::instance()->setNetworkRegistrationStatus("registered");
    QTRY_COMPARE(signalSpy.count(), 1);
    presence = signalSpy.first().first().value<Tp::Presence>();
    QCOMPARE(presence.type(), Tp::ConnectionPresenceTypeAvailable);
}

QTEST_MAIN(ConnectionTest)
#include "ConnectionTest.moc"
