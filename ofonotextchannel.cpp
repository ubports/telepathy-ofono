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

// ofono-qt
#include <ofonomessage.h>

// telepathy-ofono
#include "ofonotextchannel.h"
#include "pendingmessagesmanager.h"

QDBusArgument &operator<<(QDBusArgument &argument, const AttachmentStruct &attachment)
{
    argument.beginStructure();
    argument << attachment.id << attachment.contentType << attachment.filePath << attachment.offset << attachment.length;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, AttachmentStruct &attachment)
{
    argument.beginStructure();
    argument >> attachment.id >> attachment.contentType >> attachment.filePath >> attachment.offset >> attachment.length;
    argument.endStructure();
    return argument;
}

oFonoTextChannel::oFonoTextChannel(oFonoConnection *conn, QString phoneNumber, uint targetHandle, QObject *parent):
    QObject(parent),
    mConnection(conn),
    mPhoneNumber(phoneNumber),
    mTargetHandle(targetHandle),
    mMessageCounter(1)
{
    qDBusRegisterMetaType<AttachmentStruct>();
    qDBusRegisterMetaType<AttachmentList>();

    Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(mConnection,
                                                             TP_QT_IFACE_CHANNEL_TYPE_TEXT,
                                                             targetHandle,
                                                             Tp::HandleTypeContact);
    Tp::BaseChannelTextTypePtr textType = Tp::BaseChannelTextType::create(baseChannel.data());
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(textType));

    QStringList supportedContentTypes = QStringList() << "text/plain";

    Tp::UIntList messageTypes = Tp::UIntList() << 
                                Tp::ChannelTextMessageTypeNormal << 
                                Tp::ChannelTextMessageTypeDeliveryReport;
    uint messagePartSupportFlags = 0;
    uint deliveryReportingSupport = Tp::DeliveryReportingSupportFlagReceiveSuccesses;
    mMessagesIface = Tp::BaseChannelMessagesInterface::create(textType.data(),
                                                          supportedContentTypes,
                                                          messageTypes,
                                                          messagePartSupportFlags,
                                                          deliveryReportingSupport);

    mMessagesIface->setSendMessageCallback(Tp::memFun(this,&oFonoTextChannel::sendMessage));

    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mMessagesIface));
    mBaseChannel = baseChannel;
    mTextChannel = Tp::BaseChannelTextTypePtr::dynamicCast(mBaseChannel->interface(TP_QT_IFACE_CHANNEL_TYPE_TEXT));
    mTextChannel->setMessageAcknowledgedCallback(Tp::memFun(this,&oFonoTextChannel::messageAcknowledged));
    QObject::connect(mBaseChannel.data(), SIGNAL(closed()), this, SLOT(deleteLater()));
}

oFonoTextChannel::~oFonoTextChannel()
{
}

Tp::BaseChannelPtr oFonoTextChannel::baseChannel()
{
    return mBaseChannel;
}

void oFonoTextChannel::sendDeliveryReport(const QString &messageId, Tp::DeliveryStatus status)
{
    Tp::MessagePartList partList;
    Tp::MessagePart header;
    header["message-sender"] = QDBusVariant(mTargetHandle);
    header["message-sender-id"] = QDBusVariant(mPhoneNumber);
    header["message-type"] = QDBusVariant(Tp::ChannelTextMessageTypeDeliveryReport);
    header["delivery-status"] = QDBusVariant(status);
    header["delivery-token"] = QDBusVariant(messageId);
    partList << header;
    mTextChannel->addReceivedMessage(partList);
}

void oFonoTextChannel::deliveryReportReceived(const QString &messageId, bool success)
{
    sendDeliveryReport(messageId, success ? Tp::DeliveryStatusDelivered : Tp::DeliveryStatusPermanentlyFailed);
}

void oFonoTextChannel::messageAcknowledged(const QString &id)
{
    Q_EMIT messageRead(id);
}

QString oFonoTextChannel::sendMessage(const Tp::MessagePartList& message, uint flags, Tp::DBusError* error)
{
    bool success = true;
    Tp::MessagePart header = message.at(0);
    Tp::MessagePart body = message.at(1);

    QString objpath = mConnection->messageManager()->sendMessage(mPhoneNumber, body["content"].variant().toString(), success).path();
    if (objpath.isEmpty() || !success) {
        if (!success) {
            qWarning() << mConnection->messageManager()->errorName() << mConnection->messageManager()->errorMessage();
        } else {
            error->set(TP_QT_ERROR_INVALID_ARGUMENT, mConnection->messageManager()->errorMessage());
        }
    }
    PendingMessagesManager::instance()->addPendingMessage(objpath, mPhoneNumber);

    OfonoMessage *msg = new OfonoMessage(objpath);
    QObject::connect(msg, SIGNAL(stateChanged(QString)), SLOT(onOfonoMessageStateChanged(QString)));
    return objpath;
}

void oFonoTextChannel::onOfonoMessageStateChanged(QString status)
{
    OfonoMessage *msg = static_cast<OfonoMessage *>(sender());
    if(msg) {
        Tp::DeliveryStatus delivery_status;
        if (status == "sent") {
            delivery_status = Tp::DeliveryStatusAccepted;
            msg->deleteLater();
        } else if(status == "failed") {
            delivery_status = Tp::DeliveryStatusPermanentlyFailed;
            PendingMessagesManager::instance()->removePendingMessage(msg->path());
            msg->deleteLater();
        } else if(status == "pending") {
            delivery_status = Tp::DeliveryStatusTemporarilyFailed;
        } else {
            delivery_status = Tp::DeliveryStatusUnknown;
        }

        sendDeliveryReport(msg->path(), delivery_status);
    }
}

void oFonoTextChannel::messageReceived(const QString &message, const QVariantMap &info)
{
    Tp::MessagePartList partList;

    Tp::MessagePart body;
    body["content-type"] = QDBusVariant("text/plain");
    body["content"] = QDBusVariant(message);

    Tp::MessagePart header;
    header["message-token"] = QDBusVariant(info["SentTime"].toString() +"-" + QString::number(mMessageCounter++));
    header["message-received"] = QDBusVariant(QDateTime::fromString(info["SentTime"].toString(), Qt::ISODate).toTime_t());
    header["message-sender"] = QDBusVariant(mTargetHandle);
    header["message-sender-id"] = QDBusVariant(mPhoneNumber);
    header["message-type"] = QDBusVariant(Tp::ChannelTextMessageTypeNormal);
    partList << header << body;

    mTextChannel->addReceivedMessage(partList);
}

void oFonoTextChannel::mmsReceived(const QString &id, const QVariantMap &properties)
{
    Tp::MessagePartList message;
    QString subject = properties["Subject"].toString();
    QString smil = properties["Smil"].toString();

    Tp::MessagePart header;
    header["message-token"] = QDBusVariant(id);
    header["message-sender"] = QDBusVariant(mTargetHandle);
    header["message-received"] = QDBusVariant(QDateTime::fromString(properties["Date"].toString(), Qt::ISODate).toTime_t());
    header["message-type"] = QDBusVariant(Tp::DeliveryStatusDelivered);
    if (!subject.isEmpty())
    {
        header["subject"] = QDBusVariant(subject);
    }
    message << header;
    AttachmentList mmsdAttachments = qdbus_cast<AttachmentList>(properties["Attachments"]);
    Q_FOREACH(const AttachmentStruct &attachment, mmsdAttachments) {
        QFile attachmentFile(attachment.filePath);
        if (!attachmentFile.open(QIODevice::ReadOnly)) {
            qWarning() << "fail to load attachment" << attachmentFile.errorString() << attachment.filePath;
            continue;
        }
        // FIXME check if we managed to read the total attachment file
        attachmentFile.seek(attachment.offset);
        QByteArray fileData = attachmentFile.read(attachment.length);
        Tp::MessagePart part;
        part["content-type"] =  QDBusVariant(attachment.contentType);
        part["identifier"] = QDBusVariant(attachment.id);
        part["content"] = QDBusVariant(fileData);
        part["size"] = QDBusVariant(attachment.length);

        message << part;
    }

    if (!smil.isEmpty()) {
        Tp::MessagePart part;
        part["content-type"] =  QDBusVariant(QString("application/smil"));
        part["identifier"] = QDBusVariant(QString("smil"));
        part["content"] = QDBusVariant(smil);
        part["size"] = QDBusVariant(smil.size());
        message << part;
    }

    mTextChannel->addReceivedMessage(message);
}
