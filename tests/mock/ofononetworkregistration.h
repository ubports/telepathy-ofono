/*
 * This file is part of ofono-qt
 *
 * Copyright (C) 2010-2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef OFONONETWORKREGISTRATION_H
#define OFONONETWORKREGISTRATION_H

#include <QtCore/QObject>
#include <QStringList>
#include <QDBusError>
#include <QDBusObjectPath>
#include "ofonomodeminterface.h"
#include "libofono-qt_global.h"

struct OfonoOperatorStruct {
    QDBusObjectPath path;
    QVariantMap properties;
};
typedef QList<OfonoOperatorStruct> OfonoOperatorList;
Q_DECLARE_METATYPE(OfonoOperatorStruct)
Q_DECLARE_METATYPE(OfonoOperatorList)

//! This class is used to access oFono network registration API
/*!
 * The API is documented in
 * http://git.kernel.org/?p=network/ofono/ofono.git;a=blob_plain;f=doc/network-api.txt
 */
class OFONO_QT_EXPORT OfonoNetworkRegistration : public OfonoModemInterface
{
    Q_OBJECT

    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(uint locationAreaCode READ locationAreaCode NOTIFY locationAreaCodeChanged)
    Q_PROPERTY(uint cellId READ cellId NOTIFY cellIdChanged)
    Q_PROPERTY(QString mcc READ mcc NOTIFY mccChanged)
    Q_PROPERTY(QString mnc READ mnc NOTIFY mncChanged)
    Q_PROPERTY(QString technology READ technology NOTIFY technologyChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(uint strength READ strength NOTIFY strengthChanged)
    Q_PROPERTY(QString baseStation READ baseStation NOTIFY baseStationChanged)
    
public:
    OfonoNetworkRegistration(OfonoModem::SelectionSetting modemSetting, const QString &modemPath, QObject *parent=0);
    ~OfonoNetworkRegistration();
    
    /* Properties */
    QString mode() const;
    QString status() const;
    uint locationAreaCode() const;
    uint cellId() const;
    QString mcc() const;
    QString mnc() const;
    QString technology() const;
    QString name() const;
    uint strength() const;
    QString baseStation() const;

public Q_SLOTS:    
    void registerOp();
    void getOperators();
    void scan();
    
Q_SIGNALS:
    void modeChanged(const QString &mode);
    void statusChanged(const QString &status);
    void locationAreaCodeChanged(uint locationAreaCode);
    void cellIdChanged(uint cellId);
    void mccChanged(const QString &mcc);
    void mncChanged(const QString &mnc);
    void technologyChanged(const QString &technology);
    void nameChanged(const QString &name);
    void strengthChanged(uint strength);
    void baseStationChanged(const QString &baseStation);

    void registerComplete(bool success);
    void getOperatorsComplete(bool success, const QStringList &operatorIds);
    void scanComplete(bool success, const QStringList &operatorIds);

private Q_SLOTS:
    void propertyChanged(const QString& property, const QVariant& value);
    void registerResp();
    void registerErr(QDBusError error);
    void getOperatorsResp(OfonoOperatorList list);
    void getOperatorsErr(QDBusError error);
    void scanResp(OfonoOperatorList list);
    void scanErr(QDBusError error);

private:

};

#endif  /* !OFONONETWORKREGISTRATION_H */
