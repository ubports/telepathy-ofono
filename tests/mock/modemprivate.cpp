/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * Authors:
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

#include <QDBusConnection>

#include "modemprivate.h"
#include "modemprivateadaptor.h"

QMap<QString, ModemPrivate*> modemData;

ModemPrivate::ModemPrivate(QObject *parent) :
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject(OFONO_MOCK_MODEM_OBJECT, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    SetProperty("Powered", QDBusVariant(QVariant(false)));
    SetProperty("Online", QDBusVariant(QVariant(false)));
    SetProperty("Lockdown", QDBusVariant(QVariant(false)));
    SetProperty("Emergency", QDBusVariant(QVariant(false)));
    SetProperty("Name", QDBusVariant(QVariant("Mock Modem")));
    SetProperty("Manufacturer", QDBusVariant(QVariant("Canonical")));
    SetProperty("Model", QDBusVariant(QVariant("Mock Modem")));
    SetProperty("Revision", QDBusVariant(QVariant("1.0")));
    SetProperty("Serial", QDBusVariant(QVariant("ABC123")));
    SetProperty("Type", QDBusVariant(QVariant("Software")));
    SetProperty("Features", QDBusVariant(QVariant(QStringList() << "gprs" << "cbs" << "net" << "sms" << "sim")));
    SetProperty("Interfaces", QDBusVariant(QVariant(QStringList() << "org.ofono.ConnectionManager" << "org.ofono.AssistedSatelliteNavigation" << "org.ofono.CellBroadcast" << "org.ofono.NetworkRegistration" << "org.ofono.CallVolume" << "org.ofono.CallMeter" << "org.ofono.SupplementaryServices" << "org.ofono.CallBarring" << "org.ofono.CallSettings" << "org.ofono.MessageWaiting" << "org.ofono.SmartMessaging" << "org.ofono.PushNotification" << "org.ofono.MessageManager" << "org.ofono.Phonebook" << "org.ofono.TextTelephony" << "org.ofono.CallForwarding" << "org.ofono.SimToolkit" << "org.ofono.NetworkTime" << "org.ofono.VoiceCallManager" << "org.ofono.SimManager")));

    new ModemAdaptor(this);
}

ModemPrivate::~ModemPrivate()
{
}

void ModemPrivate::MockSetOnline(bool online)
{
    SetProperty("Powered", QDBusVariant(QVariant(online)));
    SetProperty("Online", QDBusVariant(QVariant(online)));
}

QVariantMap ModemPrivate::GetProperties()
{
    return mProperties;
}

void ModemPrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}

