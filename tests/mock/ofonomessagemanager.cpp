/*
 * This file is part of ofono-qt
 *
 * Copyright (C) 2010-2011 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Alexander Kanavin <alex.kanavin@gmail.com>
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

#include "ofonomessagemanager.h"
#include "ofonointerface.h"
#include "messagemanagerprivate.h"

QDBusArgument &operator<<(QDBusArgument &argument, const OfonoMessageManagerStruct &message)
{
    argument.beginStructure();
    argument << message.path << message.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, OfonoMessageManagerStruct &message)
{
    argument.beginStructure();
    argument >> message.path >> message.properties;
    argument.endStructure();
    return argument;
}

OfonoMessageManager::OfonoMessageManager(OfonoModem::SelectionSetting modemSetting, const QString &modemPath, QObject *parent)
    : OfonoModemInterface(modemSetting, modemPath, "org.ofono.MessageManager", OfonoGetAllOnFirstRequest, parent)
{
    qDBusRegisterMetaType<OfonoMessageManagerStruct>();
    qDBusRegisterMetaType<OfonoMessageManagerList>();

    if (!messageManagerData.keys().contains(modem()->path())) {
        m_if->setPath(OFONO_MOCK_MESSAGE_MANAGER_OBJECT);
        messageManagerData[modem()->path()] = new MessageManagerPrivate();
    }

    m_messagelist = getMessageList();

    connect(m_if, SIGNAL(propertyChanged(const QString&, const QVariant&)), 
            this, SLOT(propertyChanged(const QString&, const QVariant&)));
    connect(m_if, SIGNAL(setPropertyFailed(const QString&)), 
            this, SLOT(setPropertyFailed(const QString&)));
    connect(m_if, SIGNAL(requestPropertyComplete(bool, const QString&, const QVariant&)),
    	    this, SLOT(requestPropertyComplete(bool, const QString&, const QVariant&)));
    connect(this, SIGNAL(validityChanged(bool)),
            this, SLOT(validityChanged(bool)));
    connect(modem(), SIGNAL(pathChanged(QString)), this, SLOT(pathChanged(const QString&)));

    connectDbusSignals(path());
}

OfonoMessageManager::~OfonoMessageManager()
{
}

void OfonoMessageManager::validityChanged(bool /*validity*/)
{
    m_messagelist = getMessageList();
}

void OfonoMessageManager::pathChanged(const QString& path)
{
    connectDbusSignals(path);
}

QStringList OfonoMessageManager::getMessageList()
{
    QDBusReply<OfonoMessageManagerList> reply;
    OfonoMessageManagerList messages;
    QDBusMessage request;
    QStringList messageList;

    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "GetMessages");
    reply = QDBusConnection::sessionBus().call(request);

    messages = reply;
    Q_FOREACH(OfonoMessageManagerStruct message, messages) {
        messageList << message.path.path();
    }
    return messageList;
}

void OfonoMessageManager::connectDbusSignals(const QString& path)
{
    QDBusConnection::sessionBus().disconnect("org.ofono", QString(), m_if->ifname(),
                                         "MessageAdded",
                                         this,
                                         SLOT(onMessageAdded(const QDBusObjectPath&, const QVariantMap&)));
    QDBusConnection::sessionBus().disconnect("org.ofono", QString(), m_if->ifname(),
                                         "MessageRemoved",
                                         this,
                                         SLOT(onMessageRemoved(const QDBusObjectPath&)));
    QDBusConnection::sessionBus().disconnect("org.ofono", QString(), m_if->ifname(),
                                         "IncomingMessage",
                                         this,
                                         SIGNAL(incomingMessage(QString, QVariantMap)));
    QDBusConnection::sessionBus().disconnect("org.ofono", QString(), m_if->ifname(),
                                         "ImmediateMessage",
                                         this,
                                         SIGNAL(immediateMessage(QString, QVariantMap)));

    QDBusConnection::sessionBus().connect("org.ofono", path, m_if->ifname(),
                                         "MessageAdded",
                                         this,
                                         SLOT(onMessageAdded(const QDBusObjectPath&, const QVariantMap&)));
    QDBusConnection::sessionBus().connect("org.ofono", path, m_if->ifname(),
                                         "MessageRemoved",
                                         this,
                                         SLOT(onMessageRemoved(const QDBusObjectPath&)));
    QDBusConnection::sessionBus().connect("org.ofono", path, m_if->ifname(),
                                         "IncomingMessage",
                                         this,
                                         SIGNAL(incomingMessage(QString, QVariantMap)));
    QDBusConnection::sessionBus().connect("org.ofono", path, m_if->ifname(),
                                         "ImmediateMessage",
                                         this,
                                         SIGNAL(immediateMessage(QString, QVariantMap)));
}

void OfonoMessageManager::requestServiceCenterAddress()
{
    m_if->requestProperty("ServiceCenterAddress");
}

void OfonoMessageManager::setServiceCenterAddress(QString address)
{
    m_if->setProperty("ServiceCenterAddress", qVariantFromValue(address));
}

void OfonoMessageManager::requestUseDeliveryReports()
{
    m_if->requestProperty("UseDeliveryReports");
}

void OfonoMessageManager::setUseDeliveryReports(bool useDeliveryReports)
{
    m_if->setProperty("UseDeliveryReports", qVariantFromValue(useDeliveryReports));
}

void OfonoMessageManager::requestBearer()
{
    m_if->requestProperty("Bearer");
}

void OfonoMessageManager::setBearer(QString bearer)
{
    m_if->setProperty("Bearer", qVariantFromValue(bearer));
}

void OfonoMessageManager::requestAlphabet()
{
    m_if->requestProperty("Alphabet");
}

void OfonoMessageManager::setAlphabet(QString alphabet)
{
    m_if->setProperty("Alphabet", qVariantFromValue(alphabet));
}


QDBusObjectPath OfonoMessageManager::sendMessage(const QString &to, const QString &message, bool &success)
{
    QDBusMessage request;
    QDBusReply<QDBusObjectPath> reply;

    request = QDBusMessage::createMethodCall("org.ofono",
                                             path(), m_if->ifname(),
                                             "SendMessage");
    request << to << message;
    reply = QDBusConnection::sessionBus().call(request);
    success = reply.isValid();
    if (!success) {
        m_if->setError(reply.error().name(), reply.error().message());
    }
    return reply;
}

void OfonoMessageManager::requestPropertyComplete(bool success, const QString& property, const QVariant& value)
{
    if (property == "ServiceCenterAddress") {
        Q_EMIT serviceCenterAddressComplete(success, value.value<QString>());
    } else if (property == "UseDeliveryReports") {
        Q_EMIT useDeliveryReportsComplete(success, value.value<bool>());
    } else if (property == "Bearer") {
        Q_EMIT bearerComplete(success, value.value<QString>());
    } else if (property == "Alphabet") {
        Q_EMIT alphabetComplete(success, value.value<QString>());
    }
}

void OfonoMessageManager::propertyChanged(const QString& property, const QVariant& value)
{
    if (property == "ServiceCenterAddress") {	
        Q_EMIT serviceCenterAddressChanged(value.value<QString>());
    } else if (property == "UseDeliveryReports") {
        Q_EMIT useDeliveryReportsChanged(value.value<bool>());
    } else if (property == "Bearer") {
        Q_EMIT bearerChanged(value.value<QString>());
    } else if (property == "Alphabet") {
        Q_EMIT alphabetChanged(value.value<QString>());
    }
}

void OfonoMessageManager::setPropertyFailed(const QString& property)
{
    if (property == "ServiceCenterAddress") {
        Q_EMIT setServiceCenterAddressFailed();
    } else if (property == "UseDeliveryReports") {
        Q_EMIT setUseDeliveryReportsFailed();
    } else if (property == "Bearer") {
        Q_EMIT setBearerFailed();
    } else if (property == "Alphabet") {
        Q_EMIT setAlphabetFailed();
    }
}

QStringList OfonoMessageManager::getMessages() const
{
    return m_messagelist;
}

void OfonoMessageManager::onMessageAdded(const QDBusObjectPath &path, const QVariantMap& /*properties*/)
{
    m_messagelist << path.path();
    Q_EMIT messageAdded(path.path());
}

void OfonoMessageManager::onMessageRemoved(const QDBusObjectPath &path)
{
    m_messagelist.removeAll(path.path());
    Q_EMIT messageRemoved(path.path());
}
