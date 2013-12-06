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

#include "voicecallmanagerprivateadaptor.h"
#include "ofonovoicecall.h"
#include "voicecallprivate.h"

QMap<QString, VoiceCallManagerPrivate*> voiceCallManagerData;

VoiceCallManagerPrivate::VoiceCallManagerPrivate(QObject *parent) :
    voiceCallCount(0),
    QObject(parent)
{
    qDBusRegisterMetaType<QMap<QDBusObjectPath, QVariantMap> >();
    QDBusConnection::sessionBus().registerObject(OFONO_MOCK_VOICECALL_MANAGER_OBJECT, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    SetProperty("EmergencyNumbers", QDBusVariant(QVariant(QStringList())));
    new VoiceCallManagerAdaptor(this);
}

VoiceCallManagerPrivate::~VoiceCallManagerPrivate()
{
}

QMap<QDBusObjectPath, QVariantMap> VoiceCallManagerPrivate::GetCalls()
{
    QMap<QDBusObjectPath, QVariantMap> props;
    Q_FOREACH(const QString &key, mVoiceCalls.keys()) {
        VoiceCallPrivate *callPrivate = voiceCallData[key];
        if (callPrivate) {
           props[QDBusObjectPath(key)] = callPrivate->GetProperties();
        }
    }
    return props;
}

QVariantMap VoiceCallManagerPrivate::GetProperties()
{
    return mProperties;
}

void VoiceCallManagerPrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    qDebug() << "VoiceCallManagerPrivate::SetProperty" << name << value.variant();
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}

QDBusObjectPath VoiceCallManagerPrivate::MockIncomingCall(const QString &from)
{
    qDebug() << "VoiceCallManagerPrivate::MockIncomingCall" << from;
    QString newPath("/OfonoVoiceCall/OfonoVoiceCall"+QString::number(++voiceCallCount));
    QDBusObjectPath newPathObj(newPath);

    QVariantMap callProperties;
    callProperties["State"] = "incoming";
    callProperties["LineIdentification"] = from;
    callProperties["Name"] = "";
    callProperties["Multiparty"] = false;
    callProperties["RemoteHeld"] = false;
    callProperties["RemoteMultiparty"] = false;
    callProperties["Emergency"] = false;

    initialCallProperties[newPath] = callProperties;

    mVoiceCalls[newPath] = new OfonoVoiceCall(newPath);
    connect(voiceCallData[newPath], SIGNAL(destroyed()), this, SLOT(onVoiceCallDestroyed()));

    Q_EMIT CallAdded(newPathObj, callProperties);

    return newPathObj;
}

void VoiceCallManagerPrivate::SwapCalls()
{
    Q_FOREACH(const QString &objPath, mVoiceCalls.keys()) {
        QDBusInterface iface("org.ofono", objPath, "org.ofono.VoiceCall");
        if (mVoiceCalls[objPath]->state() == "active") {
            iface.call("SetProperty", "State", QVariant::fromValue(QDBusVariant("held")));;
        } else if (mVoiceCalls[objPath]->state() == "held") {
            iface.call("SetProperty", "State", QVariant::fromValue(QDBusVariant("active")));;
        }
    }
}

void VoiceCallManagerPrivate::SendTones(const QString &tones)
{
    Q_EMIT TonesReceived(tones);
}

QDBusObjectPath VoiceCallManagerPrivate::Dial(const QString &to, const QString &hideCallerId)
{
    qDebug() << "DIAL" << to;
    QString newPath("/OfonoVoiceCall/OfonoVoiceCall"+QString::number(++voiceCallCount));
    QDBusObjectPath newPathObj(newPath);

    QVariantMap callProperties;
    callProperties["State"] = "dialing";
    callProperties["LineIdentification"] = to;
    callProperties["Name"] = "";
    callProperties["Multiparty"] = false;
    callProperties["RemoteHeld"] = false;
    callProperties["RemoteMultiparty"] = false;
    callProperties["Emergency"] = false;
    initialCallProperties[newPath] = callProperties;

    mVoiceCalls[newPath] = new OfonoVoiceCall(newPath);
    connect(voiceCallData[newPath], SIGNAL(destroyed()), this, SLOT(onVoiceCallDestroyed()));

    Q_EMIT CallAdded(newPathObj, callProperties);

    return newPathObj;
}

void VoiceCallManagerPrivate::onVoiceCallDestroyed()
{
    VoiceCallPrivate *voiceCall = static_cast<VoiceCallPrivate*>(sender());
    if (voiceCall) {
        voiceCallData.remove(voiceCall->objectPath());
        mVoiceCalls[voiceCall->objectPath()]->deleteLater();
        mVoiceCalls.remove(voiceCall->objectPath());
        Q_EMIT CallRemoved(QDBusObjectPath(voiceCall->objectPath()));
    }
}
