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

#include "ofonomockcontroller.h"
#include "mock/mock_common.h"

OfonoMockController::OfonoMockController(QObject *parent) :
    QObject(parent),
    mNetworkRegistrationInterface("org.ofono", OFONO_MOCK_NETWORK_REGISTRATION_OBJECT, "org.ofono.NetworkRegistration"),
    mMessageManagerInterface("org.ofono", OFONO_MOCK_MESSAGE_MANAGER_OBJECT, "org.ofono.MessageManager"),
    mVoiceCallManagerInterface("org.ofono", OFONO_MOCK_VOICECALL_MANAGER_OBJECT, "org.ofono.VoiceCallManager")
{
    QDBusConnection::sessionBus().connect("org.ofono", OFONO_MOCK_MESSAGE_MANAGER_OBJECT, "org.ofono.MessageManager", "MessageAdded", this, SIGNAL(MessageAdded(QDBusObjectPath, QVariantMap)));
    QDBusConnection::sessionBus().connect("org.ofono", OFONO_MOCK_VOICECALL_MANAGER_OBJECT, "org.ofono.VoiceCallManager", "CallAdded", this, SIGNAL(CallAdded(QDBusObjectPath, QVariantMap)));
    QDBusConnection::sessionBus().connect("org.ofono", OFONO_MOCK_VOICECALL_MANAGER_OBJECT, "org.ofono.VoiceCallManager", "TonesReceived", this, SIGNAL(TonesReceived(QString)));
    QDBusConnection::sessionBus().connect("org.ofono", OFONO_MOCK_CALL_VOLUME_OBJECT, "org.ofono.CallVolume", "PropertyChanged", this, SLOT(onCallVolumePropertyChanged(QString, QDBusVariant)));
}

OfonoMockController *OfonoMockController::instance()
{
    static OfonoMockController *self = new OfonoMockController();
    return self;
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

