/*
 * This file is part of ofono-qt
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "ofonomessagewaiting.h"
#include "ofonointerface.h"


OfonoMessageWaiting::OfonoMessageWaiting(OfonoModem::SelectionSetting modemSetting, const QString &modemPath, QObject *parent)
    : OfonoModemInterface(modemSetting, modemPath, "org.ofono.MessageWaiting", OfonoGetAllOnStartup, parent)
{
    connect(m_if, SIGNAL(propertyChanged(const QString&, const QVariant&)), 
            this, SLOT(propertyChanged(const QString&, const QVariant&)));
    connect(m_if, SIGNAL(setPropertyFailed(const QString&)), 
            this, SLOT(setPropertyFailed(const QString&)));

}

OfonoMessageWaiting::~OfonoMessageWaiting()
{
}

bool OfonoMessageWaiting::voicemailWaiting() const
{
    return m_if->properties()["VoicemailWaiting"].value<bool>();
}

int OfonoMessageWaiting::voicemailMessageCount() const
{
    return m_if->properties()["VoicemailMessageCount"].value<int>();
}

QString OfonoMessageWaiting::voicemailMailboxNumber() const
{
    return m_if->properties()["VoicemailMailboxNumber"].value<QString>();
}

void OfonoMessageWaiting::setVoicemailMailboxNumber(QString mailboxnumber)
{
    m_if->setProperty("VoicemailMailboxNumber", qVariantFromValue(mailboxnumber));
}

void OfonoMessageWaiting::setPropertyFailed(const QString& property)
{
    if (property == "VoicemailMailboxNumber")
        Q_EMIT setVoicemailMailboxNumberFailed();
}

void OfonoMessageWaiting::propertyChanged(const QString& property, const QVariant& value)
{
    if (property == "VoicemailWaiting")
        Q_EMIT voicemailWaitingChanged(value.value<bool>());
    else if (property == "VoicemailMessageCount")
        Q_EMIT voicemailMessageCountChanged(value.value<int>());
    else if (property == "VoicemailMailboxNumber")
        Q_EMIT voicemailMailboxNumberChanged(value.value<QString>());
}


