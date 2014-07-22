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

#include "mmsdmessage.h"

MMSDMessage::MMSDMessage(QString objectPath, QVariantMap properties, QObject *parent)
    : QObject(parent), 
      m_messagePath(objectPath),
      m_properties(properties)
{
    qDebug() << "MMSDMessage::MMSDMessage created" << m_messagePath;
    qDebug() << QDBusConnection::sessionBus().connect("org.ofono.mms", m_messagePath, "org.ofono.mms.Message",
                                          "PropertyChanged", this,
                                          SLOT(onPropertyChanged(QString,QVariant)));
    /*if (m_properties.isEmpty()) {
        QDBusMessage request;
        request = QDBusMessage::createMethodCall("org.ofono.mms",
                                       m_messagePath, "org.ofono.mms.Message",
                                       "GetProperties");
        QDBusReply<QVariantMap> reply = QDBusConnection::sessionBus().call(request);
        if (reply.isValid()) {
            m_properties = reply;
        }
    }*/
}

MMSDMessage::~MMSDMessage()
{
}

QString MMSDMessage::path() const
{
    return m_messagePath;
}

QVariantMap MMSDMessage::properties() const
{
    return m_properties;
}

void MMSDMessage::onPropertyChanged(const QString &property, const QVariant &value)
{
    qDebug() << "property changed" << property << value;
    m_properties[property] = value;
    Q_EMIT propertyChanged(property, value);
}

void MMSDMessage::markRead() const
{
    QDBusMessage request;
    request = QDBusMessage::createMethodCall("org.ofono.mms",
                                   m_messagePath, "org.ofono.mms.Message",
                                   "MarkRead");
    QDBusConnection::sessionBus().call(request);
}

void MMSDMessage::remove() const
{
    QDBusMessage request;
    request = QDBusMessage::createMethodCall("org.ofono.mms",
                                   m_messagePath, "org.ofono.mms.Message",
                                   "Delete");
    QDBusConnection::sessionBus().call(request);
}
