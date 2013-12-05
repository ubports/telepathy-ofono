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

#include <QDBusConnection>

#include "callvolumeprivateadaptor.h"

QMap<QString, CallVolumePrivate*> callVolumeData;

CallVolumePrivate::CallVolumePrivate(QObject *parent) :
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject(OFONO_MOCK_CALL_VOLUME_OBJECT, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    SetProperty("Muted", QDBusVariant(QVariant(false)));

    new CallVolumeAdaptor(this);
}

CallVolumePrivate::~CallVolumePrivate()
{
}

QVariantMap CallVolumePrivate::GetProperties()
{
    return mProperties;
}

void CallVolumePrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    qDebug() << "CallVolumePrivate::SetProperty" << name << value.variant();
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}

