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

#include "ofonointerface.h"

OfonoInterface::OfonoInterface(const QString& path, const QString& ifname, OfonoGetPropertySetting setting, QObject *parent)
    : QObject(parent) , m_path(path), m_ifname(ifname), m_getpropsetting(setting)
{
}

OfonoInterface::~OfonoInterface()
{
}

void OfonoInterface::setPath(const QString& path)
{
    m_path = path;
}

QVariantMap OfonoInterface::properties() const
{
    return m_properties;
}

void OfonoInterface::resetProperties()
{
    m_properties = QVariantMap();
}

QVariantMap OfonoInterface::getAllPropertiesSync()
{
    QVariantMap map;
    return map;
}

void OfonoInterface::requestProperty(const QString& name)
{
}

void OfonoInterface::getPropertiesAsyncResp(QVariantMap properties)
{
}

void OfonoInterface::getPropertiesAsyncErr(const QDBusError& error)
{
}

void OfonoInterface::onPropertyChanged(QString property, QDBusVariant value)
{
    m_properties[property] = value.variant();
    Q_EMIT propertyChanged(property, value.variant());
}

void OfonoInterface::setProperty(const QString& name, const QVariant& property, const QString& password)
{
    QTimer::singleShot(0, this, SLOT(onPropertyChanged(name, property)));
}

void OfonoInterface::setPropertyResp()
{
}

void OfonoInterface::setPropertyErr(const QDBusError& error)
{
}

void OfonoInterface::setError(const QString& errorName, const QString& errorMessage)
{
    m_errorName = errorName;
    m_errorMessage = errorMessage;
}
