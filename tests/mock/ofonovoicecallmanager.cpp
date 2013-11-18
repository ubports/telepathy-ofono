/*
 * This file is part of ofono-qt
 *
 * Copyright (C) 2010-2011 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Alexander Kanavin <alex.kanavin@gmail.com>
 *
 * Portions of this file are Copyright (C) 2011 Intel Corporation
 * Contact: Shane Bryan <shane.bryan@linux.intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <QtDBus/QtDBus>
#include <QtCore/QObject>

#include "ofonovoicecallmanager.h"
#include "ofonointerface.h"

#define DIAL_TIMEOUT 30000
#define TONE_TIMEOUT 10000
#define TRANSFER_TIMEOUT 20000
#define SWAP_TIMEOUT 20000
#define HANGUP_TIMEOUT 30000
#define HOLD_TIMEOUT 30000
#define PRIVATE_CHAT_TIMEOUT 30000
#define CREATE_MULTIPARTY_TIMEOUT 30000

QDBusArgument &operator<<(QDBusArgument &argument, const OfonoVoiceCallManagerStruct &call)
{
    argument.beginStructure();
    argument << call.path << call.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, OfonoVoiceCallManagerStruct &call)
{
    argument.beginStructure();
    argument >> call.path >> call.properties;
    argument.endStructure();
    return argument;
}

OfonoVoiceCallManager::OfonoVoiceCallManager(OfonoModem::SelectionSetting modemSetting, const QString &modemPath, QObject *parent)
    : OfonoModemInterface(modemSetting, modemPath, "org.ofono.VoiceCallManager", OfonoGetAllOnStartup, parent)
{
    qDBusRegisterMetaType<OfonoVoiceCallManagerStruct>();
    qDBusRegisterMetaType<OfonoVoiceCallManagerList>();

    m_calllist = getCallList();

    connect(m_if, SIGNAL(propertyChanged(const QString&, const QVariant&)), 
            this, SLOT(propertyChanged(const QString&, const QVariant&)));
    connect(this, SIGNAL(validityChanged(bool)),
            this, SLOT(validityChanged(bool)));
    connect(modem(), SIGNAL(pathChanged(QString)), this, SLOT(pathChanged(const QString&)));

    connectDbusSignals(path());
}

OfonoVoiceCallManager::~OfonoVoiceCallManager()
{
}


void OfonoVoiceCallManager::validityChanged(bool /*validity*/)
{
    m_calllist = getCallList();
}

void OfonoVoiceCallManager::pathChanged(const QString& path)
{
    connectDbusSignals(path);
}

QStringList OfonoVoiceCallManager::getCallList()
{
    QDBusReply<OfonoVoiceCallManagerList> reply;
    OfonoVoiceCallManagerList calls;

    QDBusMessage request;
    QStringList messageList;

    qDBusRegisterMetaType<OfonoVoiceCallManagerStruct>();
    qDBusRegisterMetaType<OfonoVoiceCallManagerList>();

    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "GetCalls");
    reply = QDBusConnection::systemBus().call(request);

    calls = reply;
    Q_FOREACH(OfonoVoiceCallManagerStruct call, calls) {
        messageList<< call.path.path();
    }
    return messageList;
}

void OfonoVoiceCallManager::connectDbusSignals(const QString& path)
{
    QDBusConnection::systemBus().disconnect("org.ofono", QString(), m_if->ifname(),
                                         "CallAdded", this,
                                         SLOT(callAddedChanged(const QDBusObjectPath&, const QVariantMap&)));
    QDBusConnection::systemBus().disconnect("org.ofono", QString(), m_if->ifname(),
                                         "CallRemoved", this,
                                         SLOT(callRemovedChanged(const QDBusObjectPath&)));
    QDBusConnection::systemBus().disconnect("org.ofono", QString(), m_if->ifname(), 
                                        "BarringActive", this,
                                        SIGNAL(barringActive(const QString&)));
    QDBusConnection::systemBus().disconnect("org.ofono", QString(), m_if->ifname(), 
                                        "Forwarded", this,
                                        SIGNAL(forwarded(const QString&)));

    QDBusConnection::systemBus().connect("org.ofono", path, m_if->ifname(),
                                         "CallAdded", this,
                                         SLOT(callAddedChanged(const QDBusObjectPath&, const QVariantMap&)));
    QDBusConnection::systemBus().connect("org.ofono", path, m_if->ifname(),
                                         "CallRemoved", this,
                                         SLOT(callRemovedChanged(const QDBusObjectPath&)));
    QDBusConnection::systemBus().connect("org.ofono", path, m_if->ifname(), 
                                        "BarringActive", this,
                                        SIGNAL(barringActive(const QString&)));
    QDBusConnection::systemBus().connect("org.ofono", path, m_if->ifname(), 
                                        "Forwarded", this,
                                        SIGNAL(forwarded(const QString&)));
}

QDBusObjectPath OfonoVoiceCallManager::dial(const QString &number, const QString &callerid_hide, bool &success)
{
    QDBusMessage request;
    QDBusReply<QDBusObjectPath> reply;
    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "Dial");
    QList<QVariant>arg;
    arg.append(QVariant(number));
    arg.append(QVariant(callerid_hide));
    request.setArguments(arg);

    reply = QDBusConnection::systemBus().call(request);
    success = reply.isValid();
    if (!success) {
        m_if->setError(reply.error().name(), reply.error().message());
    }
    return reply;
}

void OfonoVoiceCallManager::hangupAll()
{
    QDBusMessage request;
    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "HangupAll");

    QDBusConnection::systemBus().callWithCallback(request, this,
                                        SLOT(hangupAllResp()),
                                        SLOT(hangupAllErr(const QDBusError&)),
                                        HANGUP_TIMEOUT);
}

void OfonoVoiceCallManager::sendTones(const QString &tonestring)
{
    QDBusMessage request;
    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "SendTones");
    QList<QVariant>arg;
    arg.append(QVariant(tonestring));
    request.setArguments(arg);

    QDBusConnection::systemBus().callWithCallback(request, this,
                                        SLOT(sendTonesResp()),
                                        SLOT(sendTonesErr(const QDBusError&)),
                                        (TONE_TIMEOUT*tonestring.length()));
}

void OfonoVoiceCallManager::transfer()
{
    QDBusMessage request;
    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "Transfer");

    QDBusConnection::systemBus().callWithCallback(request, this,
                                        SLOT(transferResp()),
                                        SLOT(transferErr(const QDBusError&)),
                                        TRANSFER_TIMEOUT);
}

void OfonoVoiceCallManager::swapCalls()
{
    QDBusMessage request;
    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "SwapCalls");

    QDBusConnection::systemBus().callWithCallback(request, this,
                                        SLOT(swapCallsResp()),
                                        SLOT(swapCallsErr(const QDBusError&)),
                                        SWAP_TIMEOUT);
}

void OfonoVoiceCallManager::releaseAndAnswer()
{
    QDBusMessage request;
    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "ReleaseAndAnswer");

    QDBusConnection::systemBus().callWithCallback(request, this,
                                        SLOT(releaseAndAnswerResp()),
                                        SLOT(releaseAndAnswerErr(const QDBusError&)),
                                        HANGUP_TIMEOUT);
}

void OfonoVoiceCallManager::holdAndAnswer()
{
    QDBusMessage request;
    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "HoldAndAnswer");

    QDBusConnection::systemBus().callWithCallback(request, this,
                                        SLOT(holdAndAnswerResp()),
                                        SLOT(holdAndAnswerErr(const QDBusError&)),
                                        HOLD_TIMEOUT);
}

void OfonoVoiceCallManager::privateChat(const QString &call)
{
    QDBusMessage request;
    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "PrivateChat");

    QList<QVariant>arg;
    arg.append(qVariantFromValue(QDBusObjectPath(call)));
    request.setArguments(arg);
    QDBusConnection::systemBus().callWithCallback(request, this,
                                        SLOT(privateChatResp(const QList<QDBusObjectPath>&)),
                                        SLOT(privateChatErr(const QDBusError&)),
                                        PRIVATE_CHAT_TIMEOUT);
}

void OfonoVoiceCallManager::createMultiparty()
{
    QDBusMessage request;
    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "CreateMultiparty");

    QDBusConnection::systemBus().callWithCallback(request, this,
                                        SLOT(createMultipartyResp(const QList<QDBusObjectPath>&)),
                                        SLOT(createMultipartyErr(const QDBusError&)),
                                        CREATE_MULTIPARTY_TIMEOUT);
}

void OfonoVoiceCallManager::hangupMultiparty()
{
    QDBusMessage request;
    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "HangupMultiparty");

    QDBusConnection::systemBus().callWithCallback(request, this,
                                        SLOT(hangupMultipartyResp()),
                                        SLOT(hangupMultipartyErr(const QDBusError&)),
                                        HANGUP_TIMEOUT);
}

void OfonoVoiceCallManager::hangupMultipartyResp()
{
    Q_EMIT hangupMultipartyComplete(true);
}

void OfonoVoiceCallManager::hangupMultipartyErr(const QDBusError &error)
{
    m_if->setError(error.name(), error.message());
    Q_EMIT hangupMultipartyComplete(false);
}

void OfonoVoiceCallManager::createMultipartyResp(const QList<QDBusObjectPath> &paths)
{
    QStringList calls;
    Q_FOREACH(QDBusObjectPath path, paths)
	calls << path.path();
    Q_EMIT createMultipartyComplete(true, calls);
}

void OfonoVoiceCallManager::createMultipartyErr(const QDBusError &error)
{
    m_if->setError(error.name(), error.message());
    Q_EMIT createMultipartyComplete(false, QStringList());
}

void OfonoVoiceCallManager::privateChatResp(const QList<QDBusObjectPath> &paths)
{
    QStringList calls;
    Q_FOREACH(QDBusObjectPath path, paths)
	calls << path.path();
    Q_EMIT privateChatComplete(true, calls);
}

void OfonoVoiceCallManager::privateChatErr(const QDBusError &error)
{
    m_if->setError(error.name(), error.message());
    Q_EMIT privateChatComplete(false, QStringList());
}

void OfonoVoiceCallManager::holdAndAnswerResp()
{
    Q_EMIT holdAndAnswerComplete(true);
}

void OfonoVoiceCallManager::holdAndAnswerErr(const QDBusError &error)
{
    m_if->setError(error.name(), error.message());
    Q_EMIT holdAndAnswerComplete(false);
}

void OfonoVoiceCallManager::releaseAndAnswerResp()
{
    Q_EMIT releaseAndAnswerComplete(true);
}

void OfonoVoiceCallManager::releaseAndAnswerErr(const QDBusError &error)
{
    m_if->setError(error.name(), error.message());
    Q_EMIT releaseAndAnswerComplete(false);
}

void OfonoVoiceCallManager::swapCallsResp()
{
    Q_EMIT swapCallsComplete(true);
}

void OfonoVoiceCallManager::swapCallsErr(const QDBusError &error)
{
    m_if->setError(error.name(), error.message());
    Q_EMIT swapCallsComplete(false);
}

void OfonoVoiceCallManager::hangupAllResp()
{
    Q_EMIT hangupAllComplete(true);
}

void OfonoVoiceCallManager::hangupAllErr(const QDBusError &error)
{
    m_if->setError(error.name(), error.message());
    Q_EMIT hangupAllComplete(false);
}
void OfonoVoiceCallManager::sendTonesResp()
{
    Q_EMIT sendTonesComplete(true);
}

void OfonoVoiceCallManager::sendTonesErr(const QDBusError &error)
{
    m_if->setError(error.name(), error.message());
    Q_EMIT sendTonesComplete(false);
}

void OfonoVoiceCallManager::transferResp()
{
    Q_EMIT transferComplete(true);
}

void OfonoVoiceCallManager::transferErr(const QDBusError &error)
{
    m_if->setError(error.name(), error.message());
    Q_EMIT transferComplete(false);
}

QStringList OfonoVoiceCallManager::emergencyNumbers() const
{
    return m_if->properties()["EmergencyNumbers"].value<QStringList>();
}

void OfonoVoiceCallManager::propertyChanged(const QString &property, const QVariant &value)
{
    if (property == "EmergencyNumbers") {	
        Q_EMIT emergencyNumbersChanged(value.value<QStringList>());
    }
}

QStringList OfonoVoiceCallManager::getCalls() const
{
    return m_calllist;
}

void OfonoVoiceCallManager::callAddedChanged(const QDBusObjectPath &path, const QVariantMap& values)
{
    m_calllist << path.path();
    Q_EMIT callAdded(path.path(), values);
}

void OfonoVoiceCallManager::callRemovedChanged(const QDBusObjectPath &path)
{
    m_calllist.removeAll(path.path());
    Q_EMIT callRemoved(path.path());
}
