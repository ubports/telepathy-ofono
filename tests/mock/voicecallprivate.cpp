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

#include "voicecallprivateadaptor.h"

QMap<QString, VoiceCallPrivate*> voiceCallData;
QMap<QString, QVariantMap> initialCallProperties;

VoiceCallPrivate::VoiceCallPrivate(const QString &path, QObject *parent) :
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject(path, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    SetProperty("LineIdentification", QDBusVariant(QVariant("")));
    SetProperty("IncomingLine", QDBusVariant(QVariant("")));
    SetProperty("Name", QDBusVariant(QVariant("")));
    SetProperty("Multiparty", QDBusVariant(QVariant(false)));
    SetProperty("State", QDBusVariant(QVariant("")));
    SetProperty("StartTime", QDBusVariant(QVariant("")));
    SetProperty("Information", QDBusVariant(QVariant("")));
    SetProperty("Icon", QDBusVariant(QVariant("")));
    SetProperty("Emergency", QDBusVariant(QVariant(false)));
    SetProperty("RemoteHeld", QDBusVariant(QVariant(false)));
    SetProperty("RemoteMultiparty", QDBusVariant(QVariant(false)));

    QMapIterator<QString, QVariant> i(initialCallProperties[path]);
    while (i.hasNext()) {
        i.next();
        SetProperty(i.key(), QDBusVariant(i.value()));
    }

    new VoiceCallAdaptor(this);
}

VoiceCallPrivate::~VoiceCallPrivate()
{
}

QVariantMap VoiceCallPrivate::GetProperties()
{
    return mProperties;
}

void VoiceCallPrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    qDebug() << "VoiceCallPrivate::SetProperty" << name << value.variant();
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}

void VoiceCallPrivate::Hangup()
{
    SetProperty("State", QDBusVariant(QVariant("disconnected")));
    deleteLater();
}

void VoiceCallPrivate::Answer()
{
    if (mProperties["State"] == QString("incoming")) {
        SetProperty("State", QDBusVariant(QVariant("active")));
    }
}

void VoiceCallPrivate::MockSetAlerting()
{
    if (mProperties["State"] == QString("dialing")) {
        SetProperty("State", QDBusVariant(QVariant("alerting")));
    }
}

void VoiceCallPrivate::MockAnswer()
{
    if (mProperties["State"] == QString("dialing") || mProperties["State"] == QString("alerting")) {
        SetProperty("State", QDBusVariant(QVariant("active")));
    }
}


