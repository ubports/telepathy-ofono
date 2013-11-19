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

#include "ofonomodem.h"
#include "ofonointerface.h"
#include "ofonomodemmanager.h"
#include "ofonomessage.h"
#include "ofonovoicecall.h"

OfonoModem::OfonoModem(SelectionSetting setting, const QString &modemPath, QObject *parent)
	: QObject(parent), m_selectionSetting(setting)
{
    
    m_mm = new OfonoModemManager(this);
    connect(m_mm, SIGNAL(modemAdded(QString)), this, SLOT(modemAdded(QString)));
    connect(m_mm, SIGNAL(modemRemoved(QString)), this, SLOT(modemRemoved(QString)));

    QString finalModemPath;

    if (setting == AutomaticSelect)
        finalModemPath = m_mm->modems().value(0);
    else if (setting == ManualSelect)
        if (m_mm->modems().contains(modemPath))
            finalModemPath = modemPath;
    
    if (finalModemPath.isEmpty()) {
        finalModemPath = "/";
    } 
    m_if = new OfonoInterface(finalModemPath, "org.ofono.Modem", OfonoGetAllOnStartup, this);
    m_if->setProperty("Online", false);
    m_if->setProperty("Powered", false);
    m_if->setProperty("Lockdown", false);
    m_if->setProperty("Emergency", false);
    m_if->setProperty("Name", "Mock Modem");
    m_if->setProperty("Manufacturer", "Foobar");
    m_if->setProperty("Model", "Foobar");
    m_if->setProperty("Revision", "1.0");
    m_if->setProperty("Serial", "ABC123");
    m_if->setProperty("Type", "software");
    m_if->setProperty("Features", QStringList() << "gprs" << "cbs" << "net" << "sms" << "sim");
    m_if->setProperty("Interfaces", QStringList() << "org.ofono.ConnectionManager" << "org.ofono.AssistedSatelliteNavigation" << "org.ofono.CellBroadcast" << "org.ofono.NetworkRegistration" << "org.ofono.CallVolume" << "org.ofono.CallMeter" << "org.ofono.SupplementaryServices" << "org.ofono.CallBarring" << "org.ofono.CallSettings" << "org.ofono.MessageWaiting" << "org.ofono.SmartMessaging" << "org.ofono.PushNotification" << "org.ofono.MessageManager" << "org.ofono.Phonebook" << "org.ofono.TextTelephony" << "org.ofono.CallForwarding" << "org.ofono.SimToolkit" << "org.ofono.NetworkTime" << "org.ofono.VoiceCallManager" << "org.ofono.SimManager");

    connect(m_if, SIGNAL(propertyChanged(const QString&, const QVariant&)), 
            this, SLOT(propertyChanged(const QString&, const QVariant&)));
    connect(m_if, SIGNAL(setPropertyFailed(const QString&)), 
            this, SLOT(setPropertyFailed(const QString&)));
    m_isValid = m_mm->modems().contains(finalModemPath);
}

OfonoModem::~OfonoModem()
{
}

void OfonoModem::propertyChanged(const QString& property, const QVariant& value)
{
    if (property == "Online")
        Q_EMIT onlineChanged(value.value<bool>());
    else if (property == "Powered")
        Q_EMIT poweredChanged(value.value<bool>());
    else if (property == "Lockdown")
        Q_EMIT lockdownChanged(value.value<bool>());
    else if (property == "Emergency")
        Q_EMIT emergencyChanged(value.value<bool>());
    else if (property == "Name")
        Q_EMIT nameChanged(value.value<QString>());
    else if (property == "Manufacturer")
        Q_EMIT manufacturerChanged(value.value<QString>());
    else if (property == "Model")
        Q_EMIT modelChanged(value.value<QString>());
    else if (property == "Revision")
        Q_EMIT revisionChanged(value.value<QString>());
    else if (property == "Serial")
        Q_EMIT serialChanged(value.value<QString>());
    else if (property == "Type")
        Q_EMIT typeChanged(value.value<QString>());
    else if (property == "Features")
        Q_EMIT featuresChanged(value.value<QStringList>());
    else if (property == "Interfaces")
        Q_EMIT interfacesChanged(value.value<QStringList>());
}

void OfonoModem::setPropertyFailed(const QString& property)
{
    if (property == "Online")
        Q_EMIT setOnlineFailed();
    else if (property == "Powered")
        Q_EMIT setPoweredFailed();
    else if (property == "Lockdown")
        Q_EMIT setLockdownFailed();
}

void OfonoModem::modemAdded(const QString& /*modem*/)
{
    modemsChanged();
}

void OfonoModem::modemRemoved(const QString& /*modem*/)
{
    modemsChanged();
}

void OfonoModem::modemsChanged()
{
    // validity has changed
    if (isValid() != m_mm->modems().contains(path())) {
        m_isValid = m_mm->modems().contains(path());
        Q_EMIT validityChanged(isValid());
    }
    if (!m_mm->modems().contains(path())) {
        if (m_selectionSetting == AutomaticSelect) {
            QString modemPath = m_mm->modems().value(0);
            if (modemPath.isEmpty()) {
                modemPath = "/";
            }
            m_if->setPath(modemPath);
            Q_EMIT pathChanged(modemPath);
        } else if (m_selectionSetting == ManualSelect) {
            m_if->setPath("/");
        }
    }
    // validity has changed
    if (isValid() != m_mm->modems().contains(path())) {
        m_isValid = m_mm->modems().contains(path());
        Q_EMIT validityChanged(isValid());
    }
}


bool OfonoModem::isValid() const
{
    return m_isValid;
}

QString OfonoModem::path() const
{
    return m_if->path();
}

QString OfonoModem::errorName() const
{
    return m_if->errorMessage();
}

QString OfonoModem::errorMessage() const
{
    return m_if->errorMessage();
}

bool OfonoModem::powered() const
{
    return m_if->properties()["Powered"].value<bool>();
}

void OfonoModem::setPowered(bool powered)
{
    m_if->setProperty("Powered", qVariantFromValue(powered));
}

bool OfonoModem::online() const
{
    return m_if->properties()["Online"].value<bool>();
}

void OfonoModem::setOnline(bool online)
{
    m_if->setProperty("Online", qVariantFromValue(online));
}

bool OfonoModem::lockdown() const
{
    return m_if->properties()["Lockdown"].value<bool>();
}

void OfonoModem::setLockdown(bool lockdown)
{
    m_if->setProperty("Lockdown", qVariantFromValue(lockdown));
}

bool OfonoModem::emergency() const
{
    return m_if->properties()["Emergency"].value<bool>();
}

QString OfonoModem::name() const
{
    return m_if->properties()["Name"].value<QString>();
}

QString OfonoModem::manufacturer() const
{
    return m_if->properties()["Manufacturer"].value<QString>();
}

QString OfonoModem::model() const
{
    return m_if->properties()["Model"].value<QString>();
}

QString OfonoModem::revision() const
{
    return m_if->properties()["Revision"].value<QString>();
}

QString OfonoModem::serial() const
{
    return m_if->properties()["Serial"].value<QString>();
}

QString OfonoModem::type() const
{
    return m_if->properties()["Type"].value<QString>();
}

QStringList OfonoModem::features() const
{
    return m_if->properties()["Features"].value<QStringList>();
}

QStringList OfonoModem::interfaces() const
{
    return m_if->properties()["Interfaces"].value<QStringList>();
}


