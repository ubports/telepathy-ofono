/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * Authors:
 *  Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
 *  Renato Araujo Oliveira Filho <renato.filho@canonical.com>
 *  Tiago Salem Herrmann <tiago.herrmann@canonical.com>
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

#ifndef TELEPHONY_PHONEUTILS_H
#define TELEPHONY_PHONEUTILS_H

#include <QtCore/QObject>

class PhoneUtils : public QObject
{
    Q_OBJECT
public:
    enum PhoneNumberFormat {
        E164 = 0,
        International,
        National,
        RFC3966,
        Auto
    };

    PhoneUtils(QObject *parent = 0);
    ~PhoneUtils();

    static QString normalizePhoneNumber(const QString &phoneNumber);
    static bool comparePhoneNumbers(const QString &phoneNumberA,const QString &phoneNumberB);
    static bool isPhoneNumber(const QString &identifier);
    static bool isEmergencyNumber(const QString &phoneNumber);
};

#endif
