/*
 * This file is part of ofono-qt
 *
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef OFONOMESSAGE_H
#define OFONOMESSAGE_H

#include <QtCore/QObject>
#include <QVariant>
#include <QStringList>
#include <QDBusError>

#include "libofono-qt_global.h"

class OfonoInterface;

//! This class is used to access oFono message API
/*!
 * oFono message API is documented in
 * http://git.kernel.org/?p=network/ofono/ofono.git;a=blob_plain;f=doc/message-api.txt
 */
class OFONO_QT_EXPORT OfonoMessage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString path READ path)
    Q_PROPERTY(QString errorName READ errorName)
    Q_PROPERTY(QString errorMessage READ errorMessage)
    
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    
public:
    OfonoMessage(const QString &messageId, QObject *parent=0);
    OfonoMessage(const OfonoMessage &message);
    ~OfonoMessage();

    OfonoMessage operator=(const OfonoMessage &message);
    bool operator==(const OfonoMessage &message);

    //! Returns the D-Bus object path of the message object
    QString path() const;

    //! Get the D-Bus error name of the last operation.
    /*!
     * Returns the D-Bus error name of the last operation (setting a property
     * or calling a method) if it has failed
     */
    QString errorName() const;

    //! Get the D-Bus error message of the last operation.
    /*!
     * Returns the D-Bus error message of the last operation (setting a property
     * or calling a method) if it has failed
     */
    QString errorMessage() const;

    QString state() const;

Q_SIGNALS:
    void stateChanged(const QString &state);

private Q_SLOTS:
    void propertyChanged(const QString &property, const QVariant &value);

private:
    OfonoInterface *m_if;

};

#endif // OFONOMESSAGE_H
