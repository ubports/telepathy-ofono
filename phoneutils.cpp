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

#include "phoneutils_p.h"


#include <phonenumbers/phonenumbermatch.h>
#include <phonenumbers/phonenumbermatcher.h>
#include <phonenumbers/phonenumberutil.h>

#include <QLocale>
#include <QDebug>
#include <QTextStream>
#include <QFile>

QString PhoneUtils::mMcc = QString();

void PhoneUtils::setMcc(const QString &mcc)
{
    mMcc = mcc;
}

QString PhoneUtils::countryCodeForMCC(const QString &mcc, bool useFallback)
{
    static QMap<QString, QString> countryCodes;
    if (countryCodes.isEmpty()) {
        QFile countryCodesFile(":/countrycodes.txt");
        if (!countryCodesFile.open(QFile::ReadOnly)) {
            qCritical() << "Failed to open " << countryCodesFile.fileName();
            if (useFallback) {
                return region();
            }
            return QString();
        }
        QTextStream stream(&countryCodesFile);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            QStringList tuple = line.split(":");
            if (tuple.size() != 2) {
                qCritical() << "Failed to parse line" << line;
                if (useFallback) {
                    return region();
                }
                return QString();
            }
            countryCodes[tuple[0]] = tuple[1];
        }
    }
    if (!countryCodes.contains(mcc) && useFallback) {
        return region();
    }
    return countryCodes[mcc];
}

QString PhoneUtils::region()
{
     QString countryCode = QLocale::system().name().split("_").last();
     if (countryCode.size() < 2) {
         // fallback to US if no valid country code was provided, otherwise libphonenumber
         // will fail to parse any numbers
         return QString("US");
     }
     return countryCode;
}

QString PhoneUtils::normalizePhoneNumber(const QString &phoneNumber)
{
    static i18n::phonenumbers::PhoneNumberUtil *phonenumberUtil = i18n::phonenumbers::PhoneNumberUtil::GetInstance();
    if (!isPhoneNumber(phoneNumber)) {
        return phoneNumber;
    }
    std::string number = phoneNumber.toStdString();
    phonenumberUtil->NormalizeDiallableCharsOnly(&number);
    return QString::fromStdString(number);
}

bool PhoneUtils::comparePhoneNumbers(const QString &phoneNumberA, const QString &phoneNumberB)
{
    static i18n::phonenumbers::PhoneNumberUtil *phonenumberUtil = i18n::phonenumbers::PhoneNumberUtil::GetInstance();
    // if any of the number isn't a phone number, just do a simple string comparison
    if (!isPhoneNumber(phoneNumberA) || !isPhoneNumber(phoneNumberB)) {
        return phoneNumberA == phoneNumberB;
    }
    i18n::phonenumbers::PhoneNumberUtil::MatchType match = phonenumberUtil->
            IsNumberMatchWithTwoStrings(phoneNumberA.toStdString(),
                                        phoneNumberB.toStdString());
    return (match > i18n::phonenumbers::PhoneNumberUtil::NO_MATCH);
}

bool PhoneUtils::isPhoneNumber(const QString &phoneNumber)
{
    static i18n::phonenumbers::PhoneNumberUtil *phonenumberUtil = i18n::phonenumbers::PhoneNumberUtil::GetInstance();
    std::string formattedNumber;
    i18n::phonenumbers::PhoneNumber number;
    i18n::phonenumbers::PhoneNumberUtil::ErrorType error;
    error = phonenumberUtil->Parse(phoneNumber.toStdString(), countryCodeForMCC(mMcc, true).toStdString(), &number);

    switch(error) {
    case i18n::phonenumbers::PhoneNumberUtil::INVALID_COUNTRY_CODE_ERROR:
        qWarning() << "Invalid country code for:" << phoneNumber;
        return false;
    case i18n::phonenumbers::PhoneNumberUtil::NOT_A_NUMBER:
        qWarning() << "The phone number is not a valid number:" << phoneNumber;
        return false;
    case i18n::phonenumbers::PhoneNumberUtil::TOO_SHORT_AFTER_IDD:
    case i18n::phonenumbers::PhoneNumberUtil::TOO_SHORT_NSN:
    case i18n::phonenumbers::PhoneNumberUtil::TOO_LONG_NSN:
        qWarning() << "Invalid phone number" << phoneNumber;
        return false;
    default:
        break;
    }
    return true;
}
