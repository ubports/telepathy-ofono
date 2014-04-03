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

oFonoTextChannel::oFonoTextChannel(oFonoConnection *conn, QStringList phoneNumbers, bool flash, QObject *parent):
    QObject(parent),
    mConnection(conn),
    mPhoneNumbers(phoneNumbers),
    mFlash(flash),
    mMessageCounter(1)
{
    qDBusRegisterMetaType<AttachmentStruct>();
    qDBusRegisterMetaType<AttachmentList>();

    Tp::BaseChannelPtr baseChannel;
    if (phoneNumbers.size() == 1) {
        baseChannel = Tp::BaseChannel::create(mConnection,
                                              TP_QT_IFACE_CHANNEL_TYPE_TEXT,
                                              mConnection->ensureHandle(mPhoneNumbers[0]),
                                              Tp::HandleTypeContact);
    } else {
        baseChannel = Tp::BaseChannel::create(mConnection,
                                              TP_QT_IFACE_CHANNEL_TYPE_TEXT,
                                              0,
                                              Tp::HandleTypeNone);
    }
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

    mGroupIface = Tp::BaseChannelGroupInterface::create(Tp::ChannelGroupFlagCanAdd, conn->selfHandle());
    mGroupIface->setAddMembersCallback(Tp::memFun(this,&oFonoTextChannel::onAddMembers));
    mGroupIface->setRemoveMembersCallback(Tp::memFun(this,&oFonoTextChannel::onRemoveMembers));

    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mGroupIface));
    addMembers(phoneNumbers);

    mSMSIface = Tp::BaseChannelSMSInterface::create(flash, true);
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mSMSIface));

    mBaseChannel = baseChannel;
    mTextChannel = Tp::BaseChannelTextTypePtr::dynamicCast(mBaseChannel->interface(TP_QT_IFACE_CHANNEL_TYPE_TEXT));
    mTextChannel->setMessageAcknowledgedCallback(Tp::memFun(this,&oFonoTextChannel::messageAcknowledged));
    QObject::connect(mBaseChannel.data(), SIGNAL(closed()), this, SLOT(deleteLater()));
}

void oFonoTextChannel::onAddMembers(const Tp::UIntList& handles, const QString& message, Tp::DBusError* error)
{
    addMembers(mConnection->inspectHandles(Tp::HandleTypeContact, handles, error));
}

void oFonoTextChannel::onRemoveMembers(const Tp::UIntList& handles, const QString& message, Tp::DBusError* error)
{
    Q_FOREACH(uint handle, handles) {
        Q_FOREACH(const QString &phoneNumber, mConnection->inspectHandles(Tp::HandleTypeContact, Tp::UIntList() << handle, error)) {
            mPhoneNumbers.removeAll(phoneNumber);
        }
        mMembers.removeAll(handle);
    }
    mGroupIface->removeMembers(handles);
}

void oFonoTextChannel::addMembers(QStringList phoneNumbers)
{
    Tp::UIntList handles;
    Q_FOREACH(const QString &phoneNumber, phoneNumbers) {
        uint handle = mConnection->ensureHandle(phoneNumber);
        handles << handle;
        if (!mPhoneNumbers.contains(phoneNumber)) {
            mPhoneNumbers << phoneNumber;
        }
        if (!mMembers.contains(handle)) {
            mMembers << handle;
        }
    }
    mGroupIface->addMembers(handles, phoneNumbers);
}

Tp::UIntList oFonoTextChannel::members()
{
    return mMembers;
}

oFonoTextChannel::~oFonoTextChannel()
{
}

Tp::BaseChannelPtr oFonoTextChannel::baseChannel()
{
    return mBaseChannel;
}

void oFonoTextChannel::sendDeliveryReport(const QString &messageId, uint handle, Tp::DeliveryStatus status)
{
    Tp::MessagePartList partList;
    Tp::MessagePart header;
    header["message-sender"] = QDBusVariant(handle);
    header["message-sender-id"] = QDBusVariant(mPhoneNumbers[0]);
    header["message-type"] = QDBusVariant(Tp::ChannelTextMessageTypeDeliveryReport);
    header["delivery-status"] = QDBusVariant(status);
    header["delivery-token"] = QDBusVariant(messageId);
    partList << header;
    mTextChannel->addReceivedMessage(partList);
}

void oFonoTextChannel::deliveryReportReceived(const QString &messageId, uint handle, bool success)
{
    sendDeliveryReport(messageId, handle, success ? Tp::DeliveryStatusDelivered : Tp::DeliveryStatusPermanentlyFailed);
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
    QString objpath;

    if (mPhoneNumbers.size() == 1) {
        QString phoneNumber = mPhoneNumbers[0];
        uint handle = mConnection->ensureHandle(phoneNumber);
        objpath = mConnection->messageManager()->sendMessage(phoneNumber, body["content"].variant().toString(), success).path();
        if (objpath.isEmpty() || !success) {
            if (!success) {
                qWarning() << mConnection->messageManager()->errorName() << mConnection->messageManager()->errorMessage();
            } else {
                error->set(TP_QT_ERROR_INVALID_ARGUMENT, mConnection->messageManager()->errorMessage());
            }
            // create an unique id for this message that ofono failed to send
            objpath = QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + "-" + QString::number(mMessageCounter++);
            mPendingDeliveryReportFailed[objpath] = handle;
            QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
            return objpath;
        }
        OfonoMessage *msg = new OfonoMessage(objpath);
        if (msg->state() == "") {
            // message was already sent or failed too fast (this case is only reproducible with the emulator)
            msg->deleteLater();
            mPendingDeliveryReportDelivered[objpath] = handle;
            QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
            return objpath;
        }
        // FIXME: track pending messages only if delivery reports are enabled. We need a system config option for it.
        PendingMessagesManager::instance()->addPendingMessage(objpath, mPhoneNumbers[0]);
        QObject::connect(msg, SIGNAL(stateChanged(QString)), SLOT(onOfonoMessageStateChanged(QString)));
        return objpath;
    } else {
        bool someMessageSent = false;
        QString lastPhoneNumber;
        Q_FOREACH(const QString &phoneNumber, mPhoneNumbers) {
            uint handle = mConnection->ensureHandle(mPhoneNumbers[0]);
            objpath = mConnection->messageManager()->sendMessage(phoneNumber, body["content"].variant().toString(), success).path();
            lastPhoneNumber = phoneNumber;
            // dont fail if this is a group chat as we cannot track individual messages
            if (objpath.isEmpty() || !success) {
                if (!success) {
                    qWarning() << mConnection->messageManager()->errorName() << mConnection->messageManager()->errorMessage();
                } else {
                    error->set(TP_QT_ERROR_INVALID_ARGUMENT, mConnection->messageManager()->errorMessage());
                }
                continue;
            }
            someMessageSent = true;
        }
        if (!someMessageSent) {
            // for group chat we only fail if all the messages failed to send
            objpath = QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + "-" + QString::number(mMessageCounter++);
            uint handle = mConnection->ensureHandle(lastPhoneNumber);
            mPendingDeliveryReportFailed[objpath] = handle;
            QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
            return objpath;
        }
        OfonoMessage *msg = new OfonoMessage(objpath);
        if (msg->state() == "") {
            // message was already sent or failed too fast (this case is only reproducible with the emulator)
            msg->deleteLater();
            uint handle = mConnection->ensureHandle(lastPhoneNumber);
            mPendingDeliveryReportDelivered[objpath] = handle;
            QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
            // return only the last one in case of group chat for history purposes
            return objpath;
        }
        QObject::connect(msg, SIGNAL(stateChanged(QString)), SLOT(onOfonoMessageStateChanged(QString)));
        return objpath;
    }
}

void oFonoTextChannel::onProcessPendingDeliveryReport()
{
    QMapIterator<QString, uint> iterator(mPendingDeliveryReportFailed);
    while(iterator.hasNext()) {
        iterator.next();
        sendDeliveryReport(iterator.key(), iterator.value(), Tp::DeliveryStatusPermanentlyFailed);
    }
    iterator = mPendingDeliveryReportDelivered;
    while(iterator.hasNext()) {
        iterator.next();
        sendDeliveryReport(iterator.key(), iterator.value(), Tp::DeliveryStatusPermanentlyFailed);
    }

    mPendingDeliveryReportFailed.clear();
    mPendingDeliveryReportDelivered.clear();
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

        sendDeliveryReport(msg->path(), mConnection->ensureHandle(mPhoneNumbers[0]), delivery_status);
    }
}

void oFonoTextChannel::messageReceived(const QString &message, uint handle, const QVariantMap &info)
{
    Tp::MessagePartList partList;

    Tp::MessagePart body;
    body["content-type"] = QDBusVariant("text/plain");
    body["content"] = QDBusVariant(message);

    Tp::MessagePart header;
    header["message-token"] = QDBusVariant(info["SentTime"].toString() +"-" + QString::number(mMessageCounter++));
    header["message-received"] = QDBusVariant(QDateTime::fromString(info["SentTime"].toString(), Qt::ISODate).toTime_t());
    header["message-sender"] = QDBusVariant(handle);
    header["message-sender-id"] = QDBusVariant(mPhoneNumbers[0]);
    header["message-type"] = QDBusVariant(Tp::ChannelTextMessageTypeNormal);
    partList << header << body;

    mTextChannel->addReceivedMessage(partList);
}

void oFonoTextChannel::mmsReceived(const QString &id, uint handle, const QVariantMap &properties)
{
    Tp::MessagePartList message;
    QString subject = properties["Subject"].toString();
    QString smil = properties["Smil"].toString();

    Tp::MessagePart header;
    header["message-token"] = QDBusVariant(id);
    header["message-sender"] = QDBusVariant(handle);
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
