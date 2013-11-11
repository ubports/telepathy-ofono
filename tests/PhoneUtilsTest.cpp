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

#include "phoneutils_p.h"


class PhoneUtilsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testIsPhoneNumber_data();
    void testIsPhoneNumber();
    void testComparePhoneNumbers_data();
    void testComparePhoneNumbers();
};

void PhoneUtilsTest::testIsPhoneNumber_data()
{
    QTest::addColumn<QString>("number");
    QTest::addColumn<bool>("expectedResult");

    QTest::newRow("simple number") << "12345678" << true;
    QTest::newRow("number with dash") << "1234-5678" << true;
    QTest::newRow("number with area code") << "(123)12345678" << true;
    QTest::newRow("number with extension") << "12345678#123" << true;
    QTest::newRow("number with comma") << "33333333,1,1" << true;
    QTest::newRow("number with semicolon") << "33333333;1" << true;
    QTest::newRow("short/emergency number") << "190" << true;
    QTest::newRow("non phone numbers") << "abcdefg" << false;
}

void PhoneUtilsTest::testIsPhoneNumber()
{
    QFETCH(QString, number);
    QFETCH(bool, expectedResult);

    bool result = PhoneUtils::isPhoneNumber(number);
    QCOMPARE(result, expectedResult);
}

void PhoneUtilsTest::testComparePhoneNumbers_data()
{
    QTest::addColumn<QString>("number1");
    QTest::addColumn<QString>("number2");
    QTest::addColumn<bool>("expectedResult");

    QTest::newRow("string equal") << "12345678" << "12345678" << true;
    QTest::newRow("number with dash") << "1234-5678" << "12345678" << true;
    QTest::newRow("number with area code") << "12312345678" << "12345678" << true;
    QTest::newRow("number with extension") << "12345678#123" << "12345678" << false;
    QTest::newRow("both numbers with extension") << "(123)12345678#1" << "12345678#1" << true;
    QTest::newRow("numbers with different extension") << "1234567#1" << "1234567#2" << false;
    QTest::newRow("number with comma") << "33333333,1,1" << "33333333" << true;
    QTest::newRow("both numbers with comma") << "22222222,1" << "22222222,2,1" << true;
    QTest::newRow("number with semicolon") << "33333333;1" << "33333333" << true;
    QTest::newRow("both numbers with semicolon") << "22222222;1" << "22222222;2" << true;
    QTest::newRow("short/emergency numbers") << "190" << "190" << true;
    QTest::newRow("different numbers") << "12345678" << "1234567" << false;
    QTest::newRow("both non phone numbers") << "abcdefg" << "abcdefg" << true;
    QTest::newRow("different non phone numbers") << "abcdefg" << "bcdefg" << false;
    QTest::newRow("phone number and custom string") << "abc12345678" << "12345678" << false;
    // FIXME: check what other cases we need to test here"
}

void PhoneUtilsTest::testComparePhoneNumbers()
{
    QFETCH(QString, number1);
    QFETCH(QString, number2);
    QFETCH(bool, expectedResult);

    bool result = PhoneUtils::comparePhoneNumbers(number1, number2);
    QCOMPARE(result, expectedResult);
}

QTEST_MAIN(PhoneUtilsTest)
#include "PhoneUtilsTest.moc"
