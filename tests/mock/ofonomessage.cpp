/*
 * This file is part of ofono-qt
 *
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "ofonointerface.h"
#include "ofonomessage.h"
#include "messageprivate.h"


OfonoMessage::OfonoMessage(const QString& messageId, QObject *parent)
    : QObject(parent)
{
    m_if = new OfonoInterface(messageId, "org.ofono.Message", OfonoGetAllOnStartup, this);
    if (!messageData.keys().contains(messageId)) {
        messageData[messageId] = new MessagePrivate(this, m_if);
    }

    connect(m_if, SIGNAL(propertyChanged(const QString&, const QVariant&)),
            this, SLOT(propertyChanged(const QString&, const QVariant&)));
}

OfonoMessage::OfonoMessage(const OfonoMessage& message)
    : QObject(message.parent())
{
    m_if = new OfonoInterface(message.path(), "org.ofono.Message", OfonoGetAllOnStartup, this);
    if (!messageData.keys().contains(message.path())) {
        messageData[message.path()] = new MessagePrivate(this, m_if);
    }

    connect(m_if, SIGNAL(propertyChanged(const QString&, const QVariant&)),
            this, SLOT(propertyChanged(const QString&, const QVariant&)));
}

bool OfonoMessage::operator==(const OfonoMessage &message)
{
    return path() == message.path();
}

OfonoMessage::~OfonoMessage()
{
}

QString OfonoMessage::state() const
{
    return m_if->properties()["State"].value<QString>();
}

void OfonoMessage::propertyChanged(const QString &property, const QVariant &value)
{
    if (property == "State") {
        Q_EMIT stateChanged(value.value<QString>());
    }
}

QString OfonoMessage::path() const
{
    return m_if->path();
}

QString OfonoMessage::errorName() const
{
    return m_if->errorName();
}

QString OfonoMessage::errorMessage() const
{
    return m_if->errorMessage();
}
