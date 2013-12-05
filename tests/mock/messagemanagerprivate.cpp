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

#include "messagemanagerprivateadaptor.h"
#include "ofonomessage.h"

QMap<QString, MessageManagerPrivate*> messageManagerData;

MessageManagerPrivate::MessageManagerPrivate(QObject *parent) :
    messageCount(0),
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject(OFONO_MOCK_MESSAGE_MANAGER_OBJECT, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    SetProperty("ServiceCenterAddress", QDBusVariant(QVariant("")));
    SetProperty("UseDeliveryReports", QDBusVariant(QVariant(false)));
    SetProperty("Bearer", QDBusVariant(QVariant("")));
    SetProperty("Alphabet", QDBusVariant(QVariant("")));
    new MessageManagerAdaptor(this);
}

MessageManagerPrivate::~MessageManagerPrivate()
{
}

QVariantMap MessageManagerPrivate::GetProperties()
{
    return mProperties;
}

void MessageManagerPrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    qDebug() << "MessageManagerPrivate::SetProperty" << name << value.variant();
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}

void MessageManagerPrivate::MockSendMessage(const QString &from, const QString &text)
{
    QVariantMap properties;
    properties["Sender"] = from;
    properties["SentTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    properties["LocalSentTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    Q_EMIT IncomingMessage(text, properties);
}

QDBusObjectPath MessageManagerPrivate::SendMessage(const QString &to, const QString &text)
{
    QString newPath("/OfonoMessage"+QString::number(++messageCount));
    QDBusObjectPath newPathObj(newPath);
    mMessages[newPath] = new OfonoMessage(newPath);
    connect(mMessages[newPath], SIGNAL(destroyed()), this, SLOT(onMessageDestroyed()));

    Q_EMIT MessageAdded(newPathObj, QVariantMap());

    return newPathObj;
}

void MessageManagerPrivate::onMessageDestroyed()
{
    OfonoMessage *message = static_cast<OfonoMessage*>(sender());
    if (message) {
        mMessages.remove(message->path());
        Q_EMIT MessageRemoved(QDBusObjectPath(message->path()));
    }
}
