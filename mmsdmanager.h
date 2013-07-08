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

#ifndef MMSDMANAGER_H
#define MMSDMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QDBusObjectPath>
#include <QStringList>

class MMSDManager : public QObject
{
    Q_OBJECT
public:
    MMSDManager(QObject *parent=0);
    ~MMSDManager();

    QStringList services() const;

Q_SIGNALS:
    void serviceAdded(const QString &servicePath);
    void serviceRemoved(const QString &servicePath);

private Q_SLOTS:
    void onServiceAdded(const QDBusObjectPath &path, const QVariantMap &properties);
    void onServiceRemoved(const QDBusObjectPath &path);

private:
    QStringList m_services;
};

#endif
