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

#define GET_PROPERTIES_TIMEOUT 300000
#define SET_PROPERTY_TIMEOUT 300000

OfonoInterface::OfonoInterface(const QString& path, const QString& ifname, OfonoGetPropertySetting setting, QObject *parent)
    : QObject(parent) , m_path(path), m_ifname(ifname), m_getpropsetting(setting)
{
    QDBusConnection::sessionBus().connect("org.ofono", path, ifname, 
					     "PropertyChanged",
					     this,
					     SLOT(onPropertyChanged(QString, QDBusVariant)));
    if (setting == OfonoGetAllOnStartup && path != "/")
        m_properties = getAllPropertiesSync();
}

OfonoInterface::~OfonoInterface()
{
}

void OfonoInterface::setPath(const QString& path)
{
    QDBusConnection::sessionBus().disconnect("org.ofono", m_path, m_ifname, 
					     "PropertyChanged",
					     this,
					     SLOT(onPropertyChanged(QString, QDBusVariant)));
    m_path = path;
    QDBusConnection::sessionBus().connect("org.ofono", m_path, m_ifname, 
					     "PropertyChanged",
					     this,
					     SLOT(onPropertyChanged(QString, QString)));

    if (m_getpropsetting == OfonoGetAllOnStartup)
        m_properties = getAllPropertiesSync();
    else
    	resetProperties();
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
    QDBusReply<QVariantMap> reply;
    QVariantMap map;
    QDBusMessage request;

    request = QDBusMessage::createMethodCall("org.ofono",
                                             m_path, m_ifname,
                                             "GetProperties");
    reply = QDBusConnection::sessionBus().call(request);
    map = reply;
    Q_FOREACH (QString property, map.keys()) {
        Q_EMIT propertyChanged(property, map[property]);
    }
    return map;
}

void OfonoInterface::requestProperty(const QString& name)
{
    if (m_pendingProperty.length() > 0) {
        // FIXME: should indicate that a get/setProperty is already in progress
        setError(QString(), QString("Already in progress"));
        Q_EMIT requestPropertyComplete(false, name, QVariant());
        return;
    }
    
    if (m_properties.keys().contains(name)) {
        Q_EMIT requestPropertyComplete(true, name, m_properties[name]);
        return;
    }
    
    QDBusMessage request;

    request = QDBusMessage::createMethodCall("org.ofono",
					     m_path, m_ifname,
					     "GetProperties");

    bool result = QDBusConnection::sessionBus().callWithCallback(request, this,
					SLOT(getPropertiesAsyncResp(QVariantMap)),
					SLOT(getPropertiesAsyncErr(const QDBusError&)),
					GET_PROPERTIES_TIMEOUT);
    if (!result) {
        // FIXME: should indicate that sending a message failed
        setError(QString(), QString("Sending a message failed"));
        Q_EMIT requestPropertyComplete(false, name, QVariant());
    	return;
    }
    m_pendingProperty = name;
}

void OfonoInterface::getPropertiesAsyncResp(QVariantMap properties)
{
    QString prop = m_pendingProperty;
    m_properties = properties;
    m_pendingProperty = QString();
    if (m_properties.keys().contains(prop)) {
        Q_EMIT requestPropertyComplete(true, prop, m_properties[prop]);
    } else {
        // FIXME: should indicate that property is not available
        setError(QString(), QString("Property not available"));
        Q_EMIT requestPropertyComplete(false, prop, QVariant());
    }
    Q_FOREACH (QString property, properties.keys()) {
        Q_EMIT propertyChanged(property, properties[property]);
    }
}

void OfonoInterface::getPropertiesAsyncErr(const QDBusError& error)
{
    QString prop = m_pendingProperty;
    setError(error.name(), error.message());
    m_pendingProperty = QString();
    Q_EMIT requestPropertyComplete(false, prop, QVariant());
}

void OfonoInterface::onPropertyChanged(QString property, QDBusVariant value)
{
    qDebug() << "OfonoInterface::onPropertyChanged" << property << value.variant();
    m_properties[property] = value.variant();
    Q_EMIT propertyChanged(property, value.variant());
}

void OfonoInterface::setProperty(const QString& name, const QVariant& property, const QString& password)
{
    if (m_pendingProperty.length() > 0) {
        // FIXME: should indicate that a get/setProperty is already in progress
        setError(QString(), QString("Already in progress"));
        Q_EMIT setPropertyFailed(name);
        return;
    }

    QDBusMessage request;
    request = QDBusMessage::createMethodCall("org.ofono",
					     m_path, m_ifname,
					     "SetProperty");

    QVariantList arguments;
    arguments << QVariant(name) << QVariant::fromValue(QDBusVariant(property));
    if (!password.isNull())
        arguments << QVariant(password);

    request.setArguments(arguments);
    bool result = QDBusConnection::sessionBus().callWithCallback(request, this,
    					SLOT(setPropertyResp()),
    					SLOT(setPropertyErr(const QDBusError&)),
    					SET_PROPERTY_TIMEOUT);
    if (!result) {
        // FIXME: should indicate that sending a message failed
        setError(QString(), QString("Sending a message failed"));
    	Q_EMIT setPropertyFailed(name);
    	return;
    }
    m_pendingProperty = name;
}

void OfonoInterface::setPropertyResp()
{
    m_pendingProperty = QString();
    // Q_EMIT nothing; we will get a PropertyChanged signal
}

void OfonoInterface::setPropertyErr(const QDBusError& error)
{
    QString prop = m_pendingProperty;
    setError(error.name(), error.message());
    m_pendingProperty = QString();
    Q_EMIT setPropertyFailed(prop);
}

void OfonoInterface::setError(const QString& errorName, const QString& errorMessage)
{
    m_errorName = errorName;
    m_errorMessage = errorMessage;
}
