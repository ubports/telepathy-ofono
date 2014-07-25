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

#ifndef OFONOTEXTCHANNEL_H
#define OFONOTEXTCHANNEL_H

#include <QObject>

#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/Types>
#include <TelepathyQt/DBusError>

#include "connection.h"

class oFonoConnection;

class oFonoTextChannel : public QObject
{
    Q_OBJECT
public:
    oFonoTextChannel(oFonoConnection *conn, QStringList phoneNumbers, bool flash = false, QObject *parent = 0);
    QString sendMessage(Tp::MessagePartList message, uint flags, Tp::DBusError* error);
    void messageReceived(const QString & message, uint handle, const QVariantMap &info);
    Tp::BaseChannelPtr baseChannel();
    void messageAcknowledged(const QString &id);
    void mmsReceived(const QString &id, uint handle, const QVariantMap &properties);
    void deliveryReportReceived(const QString& messageId, uint handle, bool success);
    void sendDeliveryReport(const QString &messageId, uint handle, Tp::DeliveryStatus status);
    void addMembers(QStringList phoneNumbers);
    Tp::UIntList members();
    void onAddMembers(const Tp::UIntList& handles, const QString& message, Tp::DBusError* error);
    void onRemoveMembers(const Tp::UIntList& handles, const QString& message, Tp::DBusError* error);

private Q_SLOTS:
    void onMMSPropertyChanged(QString property, QVariant value);
    void onOfonoMessageStateChanged(QString status);
    void onProcessPendingDeliveryReport();

Q_SIGNALS:
    void messageRead(const QString &id);

private:
    ~oFonoTextChannel();
    Tp::BaseChannelPtr mBaseChannel;
    QStringList mPhoneNumbers;
    oFonoConnection *mConnection;
    Tp::BaseChannelMessagesInterfacePtr mMessagesIface;
    Tp::BaseChannelGroupInterfacePtr mGroupIface;
    Tp::BaseChannelSMSInterfacePtr mSMSIface;
    Tp::BaseChannelTextTypePtr mTextChannel;
    uint mMessageCounter;
    QMap<QString, uint> mPendingDeliveryReportTemporarilyFailed;
    QMap<QString, uint> mPendingDeliveryReportPermanentlyFailed;
    QMap<QString, uint> mPendingDeliveryReportAccepted;
    QMap<QString, uint> mPendingDeliveryReportDelivered;
    QMap<QString, uint> mPendingDeliveryReportUnknown;
    Tp::UIntList mMembers;
    QMap<QString, QStringList> mFilesToRemove;
    bool mFlash;
};

#endif // OFONOTEXTCHANNEL_H
