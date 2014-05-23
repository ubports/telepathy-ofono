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

#include <QtDBus>
#include <QObject>

#include "dbustypes.h"
#include "mmsdservice.h"

QDBusArgument &operator<<(QDBusArgument &argument, const MessageStruct &message)
{
    argument.beginStructure();
    argument << message.path << message.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, MessageStruct &message)
{
    argument.beginStructure();
    argument >> message.path >> message.properties;
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument&argument, const OutgoingAttachmentStruct &attachment)
{
    argument.beginStructure();
    argument << attachment.id << attachment.contentType << attachment.filePath;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, OutgoingAttachmentStruct &attachment)
{
    argument.beginStructure();
    argument >> attachment.id >> attachment.contentType >> attachment.filePath;
    argument.endStructure();
    return argument;
}

MMSDService::MMSDService(QString objectPath, oFonoConnection* connection, QObject *parent)
    : QObject(parent), 
      m_servicePath(objectPath)
{
    QDBusReply<MessageList> replyMessages;
    QDBusReply<QVariantMap> replyProperties;

    QDBusMessage request;

    qDBusRegisterMetaType<MessageStruct>();
    qDBusRegisterMetaType<MessageList>();
    qDBusRegisterMetaType<OutgoingAttachmentList>();
    qDBusRegisterMetaType<OutgoingAttachmentStruct>();

    request = QDBusMessage::createMethodCall("org.ofono.mms",
                                             m_servicePath, "org.ofono.mms.Service",
                                             "GetProperties");
    replyProperties = QDBusConnection::sessionBus().call(request);
    m_properties = replyProperties;

    request = QDBusMessage::createMethodCall("org.ofono.mms",
                                             m_servicePath, "org.ofono.mms.Service",
                                             "GetMessages");
    replyMessages = QDBusConnection::sessionBus().call(request);

    m_messages = replyMessages;

    QDBusConnection::sessionBus().connect("org.ofono.mms", m_servicePath, "org.ofono.mms.Service",
                                          "MessageAdded", this,
                                          SLOT(onMessageAdded(const QDBusObjectPath&, const QVariantMap&)));
    QDBusConnection::sessionBus().connect("org.ofono.mms", m_servicePath, "org.ofono.mms.Service",
                                          "MessageRemoved", this,
                                          SLOT(onMessageRemoved(const QDBusObjectPath&)));
}

MMSDService::~MMSDService()
{
}

QString MMSDService::path() const
{
    return m_servicePath;
}

QVariantMap MMSDService::properties() const
{
    return m_properties;
}

MessageList MMSDService::messages() const
{
    return m_messages;
}

void MMSDService::onMessageAdded(const QDBusObjectPath &path, const QVariantMap &properties)
{
    qDebug() << "message added" << path.path() << properties;
    Q_EMIT messageAdded(path.path(), properties);
}

void MMSDService::onMessageRemoved(const QDBusObjectPath& path)
{
    qDebug() << "message removed" << path.path();
    Q_EMIT messageRemoved(path.path());
}

QDBusObjectPath MMSDService::sendMessage(QStringList recipients, OutgoingAttachmentList attachments)
{
    QDBusMessage request;
    QList<QVariant> arguments;
    QDBusReply<QDBusObjectPath> reply;
    arguments.append(recipients);
    arguments.append(QVariant::fromValue(attachments));
    request = QDBusMessage::createMethodCall("org.ofono.mms",
                                             m_servicePath, "org.ofono.mms.Service",
                                             "SendMessage");
    request.setArguments(arguments);
    reply = QDBusConnection::sessionBus().call(request);

    return reply;
}
