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

#ifndef MODEMPRIVATE_H
#define MODEMPRIVATE_H

#include <QDBusContext>
#include <QDBusAbstractAdaptor>
#include <QDBusVariant>
#include "mock_common.h"

class ModemPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.Modem")
public:
    ModemPrivate(QObject *parent = 0);
    ~ModemPrivate();
Q_SIGNALS:
    void PropertyChanged(const QString &name, const QDBusVariant& value);
public Q_SLOTS:
    void MockSetOnline(bool online);
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant& value);
private:
    QVariantMap mProperties;
};

extern QMap<QString, ModemPrivate*> modemData;

#endif
