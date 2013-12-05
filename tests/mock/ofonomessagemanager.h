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
 
#ifndef OFONOMESSAGEMANAGER_H
#define OFONOMESSAGEMANAGER_H

#include <QtCore/QObject>
#include <QDBusError>
#include <QDBusObjectPath>
#include "ofonomodeminterface.h"
#include "libofono-qt_global.h"

struct OfonoMessageManagerStruct {
    QDBusObjectPath path;
    QVariantMap properties;
};
typedef QList<OfonoMessageManagerStruct> OfonoMessageManagerList;

Q_DECLARE_METATYPE(OfonoMessageManagerStruct)
Q_DECLARE_METATYPE(OfonoMessageManagerList)

//! This class is used to access oFono message manager API
/*!
 * oFono message manager API is documented in
 * http://git.kernel.org/?p=network/ofono/ofono.git;a=blob_plain;f=doc/messagemanager-api.txt
 */
class OFONO_QT_EXPORT OfonoMessageManager : public OfonoModemInterface
{
    Q_OBJECT

public:
    OfonoMessageManager(OfonoModem::SelectionSetting modemSetting, const QString &modemPath, QObject *parent=0);
    ~OfonoMessageManager();

    Q_INVOKABLE QStringList getMessages() const;
public Q_SLOTS:
    /* Properties */
    void requestServiceCenterAddress();
    void setServiceCenterAddress(QString address);
    void requestUseDeliveryReports();
    void setUseDeliveryReports(bool useDeliveryReports);
    void requestBearer();
    void setBearer(QString bearer);
    void requestAlphabet();
    void setAlphabet(QString alphabet);

    QDBusObjectPath sendMessage(const QString &to, const QString &message, bool &success);

Q_SIGNALS:
    void serviceCenterAddressChanged(const QString &address);
    void useDeliveryReportsChanged(const bool &useDeliveryReports);
    void bearerChanged(const QString &bearer);
    void alphabetChanged(const QString &alphabet);

    void serviceCenterAddressComplete(bool success, const QString &address);
    void useDeliveryReportsComplete(bool success, const bool &useDeliveryReports);
    void bearerComplete(bool success, const QString &bearer);
    void alphabetComplete(bool success, const QString &alphabet);

    void setServiceCenterAddressFailed();
    void setUseDeliveryReportsFailed();
    void setBearerFailed();
    void setAlphabetFailed();

    void messageAdded(const QString &message);
    void messageRemoved(const QString &message);
    void immediateMessage(const QString &message, const QVariantMap &info);
    void incomingMessage(const QString &message, const QVariantMap &info);

private Q_SLOTS:
    void validityChanged(bool);
    void pathChanged(const QString& path);
    void propertyChanged(const QString &property, const QVariant &value);
    void setPropertyFailed(const QString &property);
    void requestPropertyComplete(bool success, const QString &property, const QVariant &value);
    void onMessageAdded(const QDBusObjectPath &message, const QVariantMap &properties);
    void onMessageRemoved(const QDBusObjectPath &message);

private:
    QStringList getMessageList();
    void connectDbusSignals(const QString& path);

private:
    QStringList m_messagelist;
};

#endif  /* !OFONOMESSAGEMANAGER_H */
