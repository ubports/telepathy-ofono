/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This file is part of telepathy-ofono
 *
 * history-service is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * history-service is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtCore/QObject>
#include <QtTest/QtTest>


class DummyTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testNothing();
};

void DummyTest::initTestCase()
{
}

void DummyTest::testNothing()
{
    QVERIFY(true);
}

QTEST_MAIN(DummyTest)
#include "DummyTest.moc"

