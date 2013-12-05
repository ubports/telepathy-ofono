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

#ifndef OFONOMODEMINTERFACE_H
#define OFONOMODEMINTERFACE_H

#include <QtCore/QObject>
#include <QStringList>
#include "ofonomodem.h"
#include "ofonopropertysetting.h"
#include "libofono-qt_global.h"

class OfonoInterface;

//! This class implements a generic modem interface object
/*!
 * It provides validity checking and modem binding.
 * It should not be instantiated directly; instead you should instantiate
 * interface-specific subclasses.
 */
class OFONO_QT_EXPORT OfonoModemInterface : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(bool isValid READ isValid NOTIFY validityChanged)
    Q_PROPERTY(QString path READ path)
    Q_PROPERTY(QString errorName READ errorName)
    Q_PROPERTY(QString errorMessage READ errorMessage)
    
public:

    //! Construct a modem interface object
    /*!
     * \param modemSetting modem selection setting
     * \param modemPath path to the modem (may not be significant, depending on modemSetting)
     * \param ifname d-bus interface name
     * \param propertySetting oFono d-bus properties setting
     */
    OfonoModemInterface(OfonoModem::SelectionSetting modemSetting, const QString& modemPath, const QString& ifname, OfonoGetPropertySetting propertySetting, QObject *parent=0);
    ~OfonoModemInterface();

    //! Check that the modem interface object is valid
    /*!
     * This means that a modem d-bus object
     * exists and has the d-bus interface specified in the contstructor.
     */
    bool isValid() const;
    
    //! Get the modem object that this interface belongs to.
    /*!
     * The ownership of the modem object stays with the OfonoModemInterface object.
     */
    OfonoModem *modem() const;
    
    //! Returns the D-Bus object path of the interface
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

Q_SIGNALS:
    //! Interface validity has changed
    /*!
     * This may mean that modem has become unavailable
     * (or available again) or that the modem interface has become unavailable
     * (or available again)
     */
    void validityChanged(bool validity);

private:
    bool checkValidity();
    void updateValidity();

private Q_SLOTS:
    void modemValidityChanged(bool validity);
    void interfacesChanged(const QStringList &interfaces);

protected:
    OfonoInterface *m_if;

private:
    OfonoModem *m_m;
    bool m_isValid;
};
#endif
