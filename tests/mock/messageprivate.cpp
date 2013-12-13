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

#include "messageprivateadaptor.h"

QMap<QString, MessagePrivate*> messageData;

MessagePrivate::MessagePrivate(const QString &path, QObject *parent) :
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject(path, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    SetProperty("State", QDBusVariant(QVariant("pending")));

    new MessageAdaptor(this);
}

MessagePrivate::~MessagePrivate()
{
}

QVariantMap MessagePrivate::GetProperties()
{
    return mProperties;
}

void MessagePrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    qDebug() << "MessagePrivate::SetProperty" << name << value.variant();
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}

void MessagePrivate::Cancel()
{
    SetProperty("State", QDBusVariant(QVariant("failed")));
    deleteLater();
}

void MessagePrivate::MockMarkFailed()
{
    SetProperty("State", QDBusVariant(QVariant("failed")));
    deleteLater();
}

void MessagePrivate::MockMarkSent()
{
    SetProperty("State", QDBusVariant(QVariant("sent")));
    deleteLater();
}
