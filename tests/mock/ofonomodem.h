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
 

#ifndef OFONOMODEM_H
#define OFONOMODEM_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtDBus/QDBusContext>
#include "libofono-qt_global.h"

class OfonoModemManager;
class OfonoInterface;

//! This class is used to access an oFono modem object and its properties
/*!
 * oFono modem properties are documented in
 * http://git.kernel.org/?p=network/ofono/ofono.git;a=blob_plain;f=doc/modem-api.txt
 */
class OFONO_QT_EXPORT OfonoModem : public QObject 
{

Q_OBJECT

Q_PROPERTY(bool isValid READ isValid NOTIFY validityChanged)
Q_PROPERTY(QString path READ path NOTIFY pathChanged)
Q_PROPERTY(QString errorName READ errorName)
Q_PROPERTY(QString errorMessage READ errorMessage)

Q_PROPERTY(bool powered READ powered WRITE setPowered NOTIFY poweredChanged)
Q_PROPERTY(bool online READ online WRITE setOnline NOTIFY onlineChanged)
Q_PROPERTY(bool lockdown READ lockdown WRITE setLockdown NOTIFY lockdownChanged)
Q_PROPERTY(bool emergency READ emergency NOTIFY emergencyChanged)

Q_PROPERTY(QString name READ name NOTIFY nameChanged)
Q_PROPERTY(QString manufacturer READ manufacturer NOTIFY manufacturerChanged)
Q_PROPERTY(QString model READ model NOTIFY modelChanged)
Q_PROPERTY(QString revision READ revision NOTIFY revisionChanged)
Q_PROPERTY(QString serial READ serial NOTIFY serialChanged)
Q_PROPERTY(QString type READ type NOTIFY typeChanged)

Q_PROPERTY(QStringList features READ features NOTIFY featuresChanged)
Q_PROPERTY(QStringList interfaces READ interfaces NOTIFY interfacesChanged)

public:

    //! How the modem object should select a modem
    enum SelectionSetting {
    	AutomaticSelect,	/*!< Select the first available modem automatically;
    				 * if that modem becomes unavailable, select the first available
    				 * modem again. */
    	ManualSelect 	/*!< Do not select a modem automatically,
    			 * use the modem path provided in the constructor, and do not
    			 * attempt to select another modem if the first one becomes 
    			 * unavailable. */
    };

    /*!
     * \param setting sets the modem selection policy for the object
     * \param modemPath if modem selection policy is ManualSelect, then this contains
     * the D-Bus path to the modem object. Otherwise, it is ignored.
     */
    OfonoModem(SelectionSetting setting, const QString& modemPath, QObject *parent=0);

    ~OfonoModem();

    //! Returns true if D-Bus modem object exists.
    bool isValid() const;
    
    //! Returns the D-Bus object path of the modem
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

    bool powered() const;
    bool online() const;
    bool lockdown() const;
    bool emergency() const;
    
    QString name() const;
    QString manufacturer() const;
    QString model() const;
    QString revision() const;
    QString serial() const;
    QString type() const;
    
    QStringList features() const;
    QStringList interfaces() const;

public Q_SLOTS:
    void setPowered(bool powered);
    void setOnline(bool online);
    void setLockdown(bool lockdown);

Q_SIGNALS:
    //! Issued when a modem becomes unavailable or available again
    void validityChanged(bool validity);
    //! Issued when the object has switched to another modem
    void pathChanged(QString modemPath);
    
    void poweredChanged(bool powered);
    void setPoweredFailed();
    void onlineChanged(bool online);
    void setOnlineFailed();
    void lockdownChanged(bool lockdown);
    void setLockdownFailed();
    void emergencyChanged(bool emergency);

    void nameChanged(const QString &name);
    void manufacturerChanged(const QString &manufacturer);
    void modelChanged(const QString &model);
    void revisionChanged(const QString &revision);
    void serialChanged(const QString &serial);
    void typeChanged(const QString &type);

    void featuresChanged(const QStringList &features);
    void interfacesChanged(const QStringList &interfaces);


private Q_SLOTS:
    void propertyChanged(const QString &property, const QVariant &value);
    void setPropertyFailed(const QString& property);
    void modemAdded(const QString &modem);
    void modemRemoved(const QString &modem);

private:
    void modemsChanged();

private:
    OfonoModemManager *m_mm;
    OfonoInterface *m_if;
    SelectionSetting m_selectionSetting;
    bool m_isValid;
};

#endif
