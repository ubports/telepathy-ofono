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
 * Authors: 
 *  Tiago Salem Herrmann <tiago.herrmann@canonical.com>
 *  Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
 */

#include "ofonomockcontroller.h"
#include "mock/mock_common.h"

OfonoMockController::OfonoMockController(QObject *parent) :
    QObject(parent),
    mNetworkRegistrationInterface("org.ofono", OFONO_MOCK_NETWORK_REGISTRATION_OBJECT, "org.ofono.NetworkRegistration"),
    mMessageManagerInterface("org.ofono", OFONO_MOCK_MESSAGE_MANAGER_OBJECT, "org.ofono.MessageManager"),
    mVoiceCallManagerInterface("org.ofono", OFONO_MOCK_VOICECALL_MANAGER_OBJECT, "org.ofono.VoiceCallManager"),
    mModemInterface("org.ofono", OFONO_MOCK_MODEM_OBJECT, "org.ofono.Modem"),
    mSimManagerInterface("org.ofono", OFONO_MOCK_SIM_MANAGER_OBJECT, "org.ofono.SimManager")
{
    QDBusConnection::sessionBus().connect("org.ofono", OFONO_MOCK_MESSAGE_MANAGER_OBJECT, "org.ofono.MessageManager", "MessageAdded", this, SIGNAL(MessageAdded(QDBusObjectPath, QVariantMap)));
    QDBusConnection::sessionBus().connect("org.ofono", OFONO_MOCK_VOICECALL_MANAGER_OBJECT, "org.ofono.VoiceCallManager", "CallAdded", this, SIGNAL(CallAdded(QDBusObjectPath, QVariantMap)));
    QDBusConnection::sessionBus().connect("org.ofono", OFONO_MOCK_VOICECALL_MANAGER_OBJECT, "org.ofono.VoiceCallManager", "TonesReceived", this, SIGNAL(TonesReceived(QString)));
    QDBusConnection::sessionBus().connect("org.ofono", OFONO_MOCK_CALL_VOLUME_OBJECT, "org.ofono.CallVolume", "PropertyChanged", this, SLOT(onCallVolumePropertyChanged(QString, QDBusVariant)));
    QDBusConnection::sessionBus().connect("org.ofono", OFONO_MOCK_SIM_MANAGER_OBJECT, "org.ofono.SimManager", "PropertyChanged", this, SLOT(onSimManagerPropertyChanged(QString, QDBusVariant)));
}

OfonoMockController *OfonoMockController::instance()
{
    static OfonoMockController *self = new OfonoMockController();
    return self;
}

void OfonoMockController::onSimManagerPropertyChanged(const QString& name, const QDBusVariant& value)
{
    if (name == "Present") {
        Q_EMIT SimManagerPresenceChanged(value.variant().value<bool>());
    }
}

void OfonoMockController::onCallVolumePropertyChanged(const QString& name, const QDBusVariant& value)
{
    if (name == "Muted") {
        Q_EMIT CallVolumeMuteChanged(value.variant().value<bool>());
    }
}

void OfonoMockController::NetworkRegistrationSetStatus(const QString &status)
{
    mNetworkRegistrationInterface.call("SetProperty", "Status", QVariant::fromValue(QDBusVariant(status)));
}

void OfonoMockController::MessageManagerSendMessage(const QString &from, const QString &text)
{
    mMessageManagerInterface.call("MockSendMessage", from, text);
}

void OfonoMockController::MessageManagerStatusReport(const QString &message, bool success)
{
    mMessageManagerInterface.call("MockStatusReport", message, success);
}

void OfonoMockController::MessageMarkSent(const QString &objPath)
{
    QDBusInterface iface("org.ofono", objPath, "org.ofono.Message");
    iface.call("MockMarkSent");
}

void OfonoMockController::MessageMarkFailed(const QString &objPath)
{
    QDBusInterface iface("org.ofono", objPath, "org.ofono.Message");
    iface.call("MockMarkFailed");
}

void OfonoMockController::MessageCancel(const QString &objPath)
{
    QDBusInterface iface("org.ofono", objPath, "org.ofono.Message");
    iface.call("Cancel");
}

void OfonoMockController::VoiceCallManagerIncomingCall(const QString &from)
{
    QDBusInterface iface("org.ofono", OFONO_MOCK_VOICECALL_MANAGER_OBJECT, "org.ofono.VoiceCallManager");
    iface.call("MockIncomingCall", from);
}

void OfonoMockController::VoiceCallManagerFailNextDtmf()
{
    QDBusInterface iface("org.ofono", OFONO_MOCK_VOICECALL_MANAGER_OBJECT, "org.ofono.VoiceCallManager");
    iface.call("MockFailNextDtmf");
}

void OfonoMockController::VoiceCallSetAlerting(const QString &objPath)
{
    QDBusInterface iface("org.ofono", objPath, "org.ofono.VoiceCall");
    iface.call("MockSetAlerting");
}

void OfonoMockController::VoiceCallAnswer(const QString &objPath)
{
    QDBusInterface iface("org.ofono", objPath, "org.ofono.VoiceCall");
    iface.call("MockAnswer");
}

void OfonoMockController::VoiceCallHangup(const QString &objPath)
{
    QDBusInterface iface("org.ofono", objPath, "org.ofono.VoiceCall");
    iface.call("Hangup");
}

void OfonoMockController::ModemSetOnline(bool online)
{
    mModemInterface.call("SetProperty", "Online", QVariant::fromValue(QDBusVariant(online)));
}

void OfonoMockController::SimManagerSetPresence(bool present)
{
    mSimManagerInterface.call("SetProperty", "Present", QVariant::fromValue(QDBusVariant(present)));
}

void OfonoMockController::SimManagerSetPinRequired(const QString &type)
{
    mSimManagerInterface.call("SetProperty", "PinRequired", QVariant::fromValue(QDBusVariant(type)));
}
