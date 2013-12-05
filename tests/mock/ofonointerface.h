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

#ifndef OFONOINTERFACE_H
#define OFONOINTERFACE_H

#include <QtCore/QObject>
#include <QVariant>
#include <QDBusVariant>
#include <QDBusError>
#include "ofonopropertysetting.h"
#include "libofono-qt_global.h"

//! Basic oFono interface class
/*!
 * This class implements basic access to properties of oFono interfaces.
 * It should not be instantiated directly; instead you should instantiate
 * interface-specific classes.
 */
class OFONO_QT_EXPORT OfonoInterface : public QObject
{
    Q_OBJECT
public:

    /*!
     * \param path D-Bus path to the interface
     * \param ifname D-Bus name of the interface
     * \param setting specifies how the object should handle oFono properties of the interface
     */
    OfonoInterface(const QString &path, const QString &ifname, OfonoGetPropertySetting setting, QObject *parent=0);
    ~OfonoInterface();

    //! Get all properties
    /*!
     * Returns the full set of current properties. If the object was constructed with
     * OfonoInterface::GetAllOnFirstRequest, and no properties have been explicitly queried yet
     * via requestProperty(), then returns nothing.
     */
    QVariantMap properties() const;
    
    //! Request a property asynchronously.
    /*! 
     * Result is returned via requestPropertyComplete() signal.
     */
    void requestProperty(const QString &name);

    //! Set a property asynchronously.
    /*!
     * Result is returned via propertyChanged() signal
     * if setting is successful or via setPropertyFailed() signal if setting has failed.
     */
    void setProperty(const QString &name, const QVariant &property, const QString& password=0);
    
    //! Resets the property cache.
    void resetProperties();
    
    //! Get the interface D-Bus path
    QString path() const {return m_path;}
    
    //! Get the interface D-Bus name
    QString ifname() const {return m_ifname;}

    //! Get the D-Bus error name of the last operation.
    /*!
     * Returns the D-Bus error name of the last operation (setting a property
     * or calling a method) if it has failed
     */
    QString errorName() const {return m_errorName;}

    //! Get the D-Bus error message of the last operation.
    /*!
     * Returns the D-Bus error message of the last operation (setting a property
     * or calling a method) if it has failed
     */
    QString errorMessage() const {return m_errorMessage;}

public Q_SLOTS:
    //! Changes the interface path
    /*!
     * This method changes the D-Bus path to the interface.
     * Properties are updated immediately if property setting is set to
     * GetAllOnStartup or reset otherwise.
     */
    void setPath(const QString &path);
    
    //! Sets the last error explicitly
    void setError(const QString &errorName, const QString &errorMessage);

Q_SIGNALS:
    //! Issued when a property has changed
    /*!
     * \param name name of the property
     * \param property value of the property
     */
    void propertyChanged(const QString &name, const QVariant &property);
    
    //! Issued when requesting a property has completed
    /*!
     * \param success true if requesting a property was successful, false if there was an error
     * \param name name of the property
     * \param property value of the property
     */
    void requestPropertyComplete(bool success, const QString &name, const QVariant &property);
    
    //! Issued when setting a property has failed
    /*!
     * \param name name of the property
     */
    void setPropertyFailed(const QString &name);

private Q_SLOTS:
    void onPropertyChanged(QString property, QDBusVariant value);
    void getPropertiesAsyncResp(QVariantMap properties);
    void getPropertiesAsyncErr(const QDBusError&);
    void setPropertyResp();
    void setPropertyErr(const QDBusError& error);
protected Q_SLOTS:
private:
    QVariantMap getAllPropertiesSync();
    
protected:
   QString m_errorName;
   QString m_errorMessage;
    
private:
   QString m_path;
   QString m_ifname;
   QVariantMap m_properties;
   QString m_pendingProperty;
   OfonoGetPropertySetting m_getpropsetting;
};

#endif
