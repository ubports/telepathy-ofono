/**
 * Copyright (C) 2015 Canonical, Ltd.
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
#include "telepathyhelper.h"
#include "ofonomockcontroller.h"

class ProtocolTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testVCardField();
};

void ProtocolTest::initTestCase()
{
    TelepathyHelper::instance();
    QSignalSpy spy(TelepathyHelper::instance(),
                   SIGNAL(accountReady()));
    QTRY_COMPARE(spy.count(), 1);
}

void ProtocolTest::testVCardField()
{
    QStringList fields = TelepathyHelper::instance()->account()->protocolInfo().addressableVCardFields();

    QCOMPARE(fields, QStringList() << "tel");
}

QTEST_MAIN(ProtocolTest)
#include "ProtocolTest.moc"
