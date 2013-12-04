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

#include "networkregistrationprivateadaptor.h"

QMap<QString, NetworkRegistrationPrivate*> networkRegistrationData;

NetworkRegistrationPrivate::NetworkRegistrationPrivate(QObject *parent) :
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject(OFONO_MOCK_NETWORK_REGISTRATION_OBJECT, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    new NetworkRegistrationAdaptor(this);
}

NetworkRegistrationPrivate::~NetworkRegistrationPrivate()
{
}

QVariantMap NetworkRegistrationPrivate::GetProperties()
{
    return mProperties;
}

void NetworkRegistrationPrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}
