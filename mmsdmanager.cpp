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

#include "mmsdmanager.h"

struct ServiceStruct {
    QDBusObjectPath path;
    QVariantMap properties;
};

typedef QList<ServiceStruct> ServiceList;
Q_DECLARE_METATYPE(ServiceStruct)
Q_DECLARE_METATYPE(ServiceList)

QDBusArgument &operator<<(QDBusArgument &argument, const ServiceStruct &service)
{
    argument.beginStructure();
    argument << service.path << service.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, ServiceStruct &service)
{
    argument.beginStructure();
    argument >> service.path >> service.properties;
    argument.endStructure();
    return argument;
}

MMSDManager::MMSDManager(QObject *parent)
    : QObject(parent)
{
    QDBusReply<ServiceList> reply;
    ServiceList services;

    QDBusMessage request;

    qDBusRegisterMetaType<ServiceStruct>();
    qDBusRegisterMetaType<ServiceList>();

    request = QDBusMessage::createMethodCall("org.ofono.mms",
                                             "/org/ofono/mms", "org.ofono.mms.Manager",
                                             "GetServices");
    reply = QDBusConnection::sessionBus().call(request);

    services = reply;
    Q_FOREACH(ServiceStruct service, services) {
        m_services << service.path.path();
    }

    QDBusConnection::sessionBus().connect("org.ofono.mms","/org/ofono/mms","org.ofono.mms.Manager",
                                          "ServiceAdded", this, 
                                          SLOT(onServiceAdded(const QDBusObjectPath&, const QVariantMap&)));
    QDBusConnection::sessionBus().connect("org.ofono.mms","/org/ofono/mms","org.ofono.mms.Manager",
                                          "ServiceRemoved", this, 
                                          SLOT(onServiceRemoved(const QDBusObjectPath&)));
}

MMSDManager::~MMSDManager()
{
}

QStringList MMSDManager::services() const
{
    return m_services;
}

void MMSDManager::onServiceAdded(const QDBusObjectPath& path, const QVariantMap& map)
{
    qDebug() << "service added" << path.path() << map;
    m_services << path.path();
    Q_EMIT serviceAdded(path.path());
}

void MMSDManager::onServiceRemoved(const QDBusObjectPath& path)
{
    qDebug() << "service removed" << path.path();
    m_services.removeAll(path.path());
    Q_EMIT serviceRemoved(path.path());
}
