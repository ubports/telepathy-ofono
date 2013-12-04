/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * Authors:
 *  Tiago Salem Herrmann <tiago.herrmann@canonical.com>
 *
 * This file is part of telepathy-ofono.
 *
 * telepathy-ofono is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * telepathy-ofono is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CALLVOLUMEPRIVATE_H
#define CALLVOLUMEPRIVATE_H

#include <QDBusContext>
#include <QDBusVariant>
#include "mock_common.h"

class CallVolumePrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.CallVolume")
public:
    CallVolumePrivate(QObject *parent = 0);
    ~CallVolumePrivate();
Q_SIGNALS:
    void PropertyChanged(const QString &name, const QDBusVariant &value);
public Q_SLOTS:
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
private:
    QVariantMap mProperties;
};

extern QMap<QString, CallVolumePrivate*> callVolumeData;

#endif
