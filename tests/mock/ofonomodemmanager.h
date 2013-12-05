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
 
#ifndef OFONOMODEMMANAGER_H
#define OFONOMODEMMANAGER_H

#include <QtCore/QObject>
#include <QVariant>
#include <QDBusObjectPath>
#include <QStringList>
#include "libofono-qt_global.h"

//! Provides access to the list of available modems and changes in that list.
class OFONO_QT_EXPORT OfonoModemManager : public QObject {

Q_OBJECT

public:

    OfonoModemManager(QObject *parent=0);

    ~OfonoModemManager();

    //! Returns a list of d-bus object paths that represent available modems
    Q_INVOKABLE QStringList modems() const;

Q_SIGNALS:
    //! Issued when a modem has been added
    void modemAdded(const QString &modemPath);
    
    //! Issued when a modem has been removed
    void modemRemoved(const QString &modemPath);

private Q_SLOTS:
    void onModemAdded(const QDBusObjectPath &path, const QVariantMap &map);
    void onModemRemoved(const QDBusObjectPath &path);

private:
    QStringList m_modems;
};

#endif
