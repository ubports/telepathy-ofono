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
    oFonoTextChannel(oFonoConnection *conn, QString phoneNumber, uint targetHandle, QObject *parent = 0);
    QString sendMessage(const Tp::MessagePartList& message, uint flags, Tp::DBusError* error);
    void messageReceived(const QString & message, const QVariantMap &info);
    Tp::BaseChannelPtr baseChannel();
    void messageAcknowledged(const QString &id);
    void mmsReceived(const QString &id, const QVariantMap &properties);
    void deliveryReportReceived(const QString& messageId, bool success);

private Q_SLOTS:
    void onOfonoMessageStateChanged(QString status);

Q_SIGNALS:
    void messageRead(const QString &id);

private:
    void sendDeliveryReport(const QString &messageId, Tp::DeliveryStatus status);
    ~oFonoTextChannel();
    Tp::BaseChannelPtr mBaseChannel;
    QString mPhoneNumber;
    oFonoConnection *mConnection;
    uint mTargetHandle;
    Tp::BaseChannelMessagesInterfacePtr mMessagesIface;
    Tp::BaseChannelTextTypePtr mTextChannel;
    uint mMessageCounter;
};

#endif // OFONOTEXTCHANNEL_H
