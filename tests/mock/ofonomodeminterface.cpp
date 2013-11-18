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

#include "ofonomodeminterface.h"
#include "ofonomodem.h"
#include "ofonointerface.h"

OfonoModemInterface::OfonoModemInterface(OfonoModem::SelectionSetting modemSetting, const QString& modemPath, const QString& ifname, OfonoGetPropertySetting propertySetting, QObject *parent)
    : QObject(parent)
{

    if (!modemData[modemPath]) {
        modemData[modemPath] = new ModemPrivate(new OfonoModem(modemSetting, modemPath, this));
    }
    m_m = modemData[modemPath]->ofonoModem();
    connect(m_m, SIGNAL(validityChanged(bool)), this, SLOT(modemValidityChanged(bool)));
    connect(m_m, SIGNAL(interfacesChanged(QStringList)), this, SLOT(interfacesChanged(QStringList)));

    m_if = new OfonoInterface(m_m->path(), ifname, propertySetting, this);
    connect(m_m, SIGNAL(pathChanged(QString)), m_if, SLOT(setPath(const QString&)));
    m_isValid = checkValidity();
}

OfonoModemInterface::~OfonoModemInterface()
{
}

bool OfonoModemInterface::isValid() const
{
    return m_isValid;
}

OfonoModem* OfonoModemInterface::modem() const
{
    return m_m;
}

bool OfonoModemInterface::checkValidity()
{
    return (m_m->isValid() && m_m->interfaces().contains(m_if->ifname()));
}

void OfonoModemInterface::updateValidity()
{
    if (isValid() != checkValidity()) {
        m_isValid = checkValidity();
        Q_EMIT validityChanged(isValid());
    }
}

void OfonoModemInterface::modemValidityChanged(bool /*validity*/)
{
    updateValidity();
}

void OfonoModemInterface::interfacesChanged(const QStringList& /*interfaces*/)
{
    updateValidity();
}

QString OfonoModemInterface::path() const
{
    return m_if->path();
}
    
QString OfonoModemInterface::errorName() const
{
    return m_if->errorName();
}

QString OfonoModemInterface::errorMessage() const
{
    return m_if->errorMessage();
}

