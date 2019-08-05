/**
 * Copyright (C) 2013-2016 Canonical, Ltd.
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
 *          Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
 */

// ofono-qt
#include <ofonomessage.h>

// telepathy-ofono
#include "ofonotextchannel.h"
#include "pendingmessagesmanager.h"

QDBusArgument &operator<<(QDBusArgument&argument, const IncomingAttachmentStruct &attachment)
{
    argument.beginStructure();
    argument << attachment.id << attachment.contentType << attachment.filePath << attachment.offset << attachment.length;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, IncomingAttachmentStruct &attachment)
{
    argument.beginStructure();
    argument >> attachment.id >> attachment.contentType >> attachment.filePath >> attachment.offset >> attachment.length;
    argument.endStructure();
    return argument;
}


oFonoTextChannel::oFonoTextChannel(oFonoConnection *conn, const QString &targetId, QStringList phoneNumbers, bool flash, QObject *parent):
    QObject(parent),
    mConnection(conn),
    mPhoneNumbers(phoneNumbers),
    mFlash(flash),
    mMessageCounter(1)
{
    qDBusRegisterMetaType<IncomingAttachmentStruct>();
    qDBusRegisterMetaType<IncomingAttachmentList>();
    bool mmsGroupChat = !targetId.isEmpty();

    Tp::BaseChannelPtr baseChannel;
    if (mmsGroupChat) {
        baseChannel = Tp::BaseChannel::create(mConnection,
                                              TP_QT_IFACE_CHANNEL_TYPE_TEXT,
                                              Tp::HandleTypeRoom,
                                              mConnection->ensureGroupHandle(targetId));
    } else if (phoneNumbers.size() == 1) {
        baseChannel = Tp::BaseChannel::create(mConnection,
                                              TP_QT_IFACE_CHANNEL_TYPE_TEXT,
                                              Tp::HandleTypeContact,
                                              mConnection->ensureHandle(mPhoneNumbers[0]));
    } else {
        // sms broadcast
        baseChannel = Tp::BaseChannel::create(mConnection,
                                              TP_QT_IFACE_CHANNEL_TYPE_TEXT);
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

    Tp::ChannelGroupFlags groupFlags = Tp::ChannelGroupFlagHandleOwnersNotAvailable |
                                       Tp::ChannelGroupFlagMembersChangedDetailed |
                                       Tp::ChannelGroupFlagProperties;

    mGroupIface = Tp::BaseChannelGroupInterface::create();
    mGroupIface->setGroupFlags(groupFlags);
    mGroupIface->setSelfHandle(mConnection->selfHandle());
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mGroupIface));

    Q_FOREACH(const QString &phoneNumber, phoneNumbers) {
        uint handle = mConnection->ensureHandle(phoneNumber);
        if (!mMembers.contains(handle)) {
            mMembers << handle;
        }
    }

    mGroupIface->setMembers(mMembers, QVariantMap());

    mSMSIface = Tp::BaseChannelSMSInterface::create(flash, true);
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mSMSIface));

    if (mmsGroupChat) {
        mRoomIface = Tp::BaseChannelRoomInterface::create("", "", "", 0, QDateTime());
        baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mRoomIface));
    }

    mBaseChannel = baseChannel;
    mTextChannel = Tp::BaseChannelTextTypePtr::dynamicCast(mBaseChannel->interface(TP_QT_IFACE_CHANNEL_TYPE_TEXT));
    mTextChannel->setMessageAcknowledgedCallback(Tp::memFun(this,&oFonoTextChannel::messageAcknowledged));
    QObject::connect(mBaseChannel.data(), &Tp::BaseChannel::closed, this, &oFonoTextChannel::deleteLater);
}

Tp::UIntList oFonoTextChannel::members()
{
    return mMembers;
}


bool oFonoTextChannel::isMultiPartMessage(const Tp::MessagePartList &message) const
{
    // multi-part messages happen in two cases:
    // - if it has more than two parts
    // - if the second part (the first is the header) is not text
    if (message.size() > 2 ||
        (message.size() == 2 && !message[1]["content-type"].variant().toString().startsWith("text/"))) {
        return true;
    }

    return false;
}

oFonoTextChannel::~oFonoTextChannel()
{
    Q_FOREACH(const QStringList &fileList, mFilesToRemove) {
        Q_FOREACH(const QString& file, fileList) {
            QFile::remove(file);
        }
    }
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

QString oFonoTextChannel::sendMessage(Tp::MessagePartList message, uint flags, Tp::DBusError* error)
{
    bool success = true;
    Tp::MessagePart header = message.at(0);
    Tp::MessagePart body = message.at(1);
    QString objpath;

    bool isRoom = baseChannel()->targetHandleType() == Tp::HandleTypeRoom;
    bool isMMS = isRoom || isMultiPartMessage(message);

    // any mms, either 1-1, group or broadcast
    if (isMMS || isRoom) {
        // pop header out
        message.removeFirst();
        OutgoingAttachmentList attachments;
        QString phoneNumber = mPhoneNumbers[0];
        uint handle = mConnection->ensureHandle(phoneNumber);
        QStringList temporaryFiles;
        Q_FOREACH(const Tp::MessagePart &part, message) {
            OutgoingAttachmentStruct attachment;
            attachment.id = part["identifier"].variant().toString();
            attachment.contentType = part["content-type"].variant().toString();
            QString attachmentsPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/telepathy-ofono/attachments";
            if (!QDir().exists(attachmentsPath) && !QDir().mkpath(attachmentsPath)) {
                qCritical() << "Failed to create attachments directory";
                objpath = QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + "-" + QString::number(mMessageCounter++);
                error->set(TP_QT_ERROR_INVALID_ARGUMENT, "Failed to create attachments to disk");
                mPendingDeliveryReportPermanentlyFailed[objpath] = handle;
                QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
                return objpath;
            }
            QTemporaryFile file(attachmentsPath + "/attachmentXXXXXX");
            file.setAutoRemove(false);
            if (!file.open()) {
                Q_FOREACH(const QString& file, temporaryFiles) {
                    QFile::remove(file);
                }
                objpath = QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + "-" + QString::number(mMessageCounter++);
                error->set(TP_QT_ERROR_INVALID_ARGUMENT, "Failed to create attachments to disk");
                mPendingDeliveryReportPermanentlyFailed[objpath] = handle;
                QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
                return objpath;
            }
            temporaryFiles << file.fileName();
            file.write(part["content"].variant().toByteArray());
            file.close();
            attachment.filePath = file.fileName();
            attachments << attachment;
        }
        // if this is a broadcast, send multiple mms
        if (!isRoom) {
            // generate an id to this broadcast operation and its delivery reports
            objpath = QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + "-" + QString::number(mMessageCounter++);
            Q_FOREACH(const QString &phoneNumber, mPhoneNumbers) {
                QString realObjpath = mConnection->sendMMS(QStringList() << phoneNumber, attachments).path();
                MMSDMessage *msg = new MMSDMessage(realObjpath, QVariantMap(), this);
                QObject::connect(msg, &MMSDMessage::propertyChanged, this, &oFonoTextChannel::onMMSPropertyChanged);
                mPendingBroadcastMMS[realObjpath] = objpath;
                mPendingDeliveryReportUnknown[objpath] = handle;
                QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
            }
            if (temporaryFiles.size() > 0 && !mFilesToRemove.contains(objpath)) {
                mFilesToRemove[objpath] = temporaryFiles;
            }
            return objpath;
        }
        objpath = mConnection->sendMMS(mPhoneNumbers, attachments).path();
        if (objpath.isEmpty()) {
            Q_FOREACH(const QString& file, temporaryFiles) {
                QFile::remove(file);
            }
            // give a temporary id for this message
            objpath = QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + "-" + QString::number(mMessageCounter++);
            // TODO: get error message from nuntium
            error->set(TP_QT_ERROR_INVALID_ARGUMENT, "Failed to send MMS");
            mPendingDeliveryReportPermanentlyFailed[objpath] = handle;
            QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
            return objpath;
        }
        MMSDMessage *msg = new MMSDMessage(objpath, QVariantMap(), this);
        QObject::connect(msg, &MMSDMessage::propertyChanged, this, &oFonoTextChannel::onMMSPropertyChanged);
        mPendingDeliveryReportUnknown[objpath] = handle;
        QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
        if (temporaryFiles.size() > 0) {
            mFilesToRemove[objpath] = temporaryFiles;
        }
        return objpath;
    }

    // 1-1 sms
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
            mPendingDeliveryReportPermanentlyFailed[objpath] = handle;
            QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
            return objpath;
        }
        OfonoMessage *msg = new OfonoMessage(objpath);
        if (msg->state() == "") {
            // message was already sent or failed too fast (this case is only reproducible with the emulator)
            msg->deleteLater();
            mPendingDeliveryReportUnknown[objpath] = handle;
            QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
            return objpath;
        }
        // FIXME: track pending messages only if delivery reports are enabled. We need a system config option for it.
        PendingMessagesManager::instance()->addPendingMessage(objpath, mPhoneNumbers[0]);
        QObject::connect(msg, &OfonoMessage::stateChanged, this, &oFonoTextChannel::onOfonoMessageStateChanged);
        return objpath;
    } else {
        // Broadcast sms
        bool someMessageSent = false;
        QString lastPhoneNumber;
        Q_FOREACH(const QString &phoneNumber, mPhoneNumbers) {
            objpath = mConnection->messageManager()->sendMessage(phoneNumber, body["content"].variant().toString(), success).path();
            lastPhoneNumber = phoneNumber;
            // dont fail if this is a broadcast chat as we cannot track individual messages
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
            mPendingDeliveryReportPermanentlyFailed[objpath] = handle;
            QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
            return objpath;
        }
        OfonoMessage *msg = new OfonoMessage(objpath);
        if (msg->state() == "") {
            // message was already sent or failed too fast (this case is only reproducible with the emulator)
            msg->deleteLater();
            uint handle = mConnection->ensureHandle(lastPhoneNumber);
            mPendingDeliveryReportUnknown[objpath] = handle;
            QTimer::singleShot(0, this, SLOT(onProcessPendingDeliveryReport()));
            // return only the last one in case of group chat for history purposes
            return objpath;
        }
        QObject::connect(msg, &OfonoMessage::stateChanged, this, &oFonoTextChannel::onOfonoMessageStateChanged);
        return objpath;
    }
}

void oFonoTextChannel::onMMSPropertyChanged(QString property, QVariant value)
{
    qDebug() << "oFonoTextChannel::onMMSPropertyChanged" << property << value;
    bool canRemoveFiles = true;
    MMSDMessage *msg = qobject_cast<MMSDMessage*>(sender());
    // FIXME - mms groupchat
    uint handle = mConnection->ensureHandle(mPhoneNumbers[0]);
    if (!msg) {
        return;
    }
    QString objectPath = msg->path();
    if (property == "Status") {
        Tp::DeliveryStatus status = Tp::DeliveryStatusUnknown;
        if (value == "Sent") {
            status = Tp::DeliveryStatusAccepted;
        } else if (value == "TransientError" || value == "PermanentError") {
            // transient error in telepathy means it is still trying, so we need to
            // set a permanent error here so the user can retry
            status = Tp::DeliveryStatusPermanentlyFailed;
        } else if (value == "draft") {
            // while it is draft we dont actually send a delivery report
            return;
        }
        if (mPendingBroadcastMMS.contains(objectPath)) {
            // if this is the last outstanding mms, we can now remove the files
            objectPath = mPendingBroadcastMMS.take(objectPath);
            QStringList originalObjPaths = mPendingBroadcastMMS.keys(objectPath);
            canRemoveFiles = originalObjPaths.size() == 0;

            if (status == Tp::DeliveryStatusAccepted) {
                // if we get at least one sucess, we notify sucess no matter if the others fail
                mPendingBroadcastFinalResult[objectPath] = true;
            }

            if (canRemoveFiles) {
                if (mPendingBroadcastFinalResult[objectPath]) {
                    status = Tp::DeliveryStatusAccepted;
                } else {
                    status = Tp::DeliveryStatusPermanentlyFailed;
                }
                Q_FOREACH(const QString& file, mFilesToRemove[objectPath]) {
                    QFile::remove(file);
                }
                mFilesToRemove.remove(objectPath);
                sendDeliveryReport(objectPath, handle, status);
                mPendingBroadcastFinalResult.remove(objectPath);
            }
            return;
        }

        Q_FOREACH(const QString& file, mFilesToRemove[objectPath]) {
            QFile::remove(file);
        }
        mFilesToRemove.remove(objectPath);
        sendDeliveryReport(objectPath, handle, status);
    }
}

void oFonoTextChannel::onProcessPendingDeliveryReport()
{
    QMapIterator<QString, uint> iterator(mPendingDeliveryReportPermanentlyFailed);
    while(iterator.hasNext()) {
        iterator.next();
        sendDeliveryReport(iterator.key(), iterator.value(), Tp::DeliveryStatusPermanentlyFailed);
    }
    iterator = mPendingDeliveryReportDelivered;
    while(iterator.hasNext()) {
        iterator.next();
        sendDeliveryReport(iterator.key(), iterator.value(), Tp::DeliveryStatusDelivered);
    }
    iterator = mPendingDeliveryReportAccepted;
    while(iterator.hasNext()) {
        iterator.next();
        sendDeliveryReport(iterator.key(), iterator.value(), Tp::DeliveryStatusAccepted);
    }
    iterator = mPendingDeliveryReportTemporarilyFailed;
    while(iterator.hasNext()) {
        iterator.next();
        sendDeliveryReport(iterator.key(), iterator.value(), Tp::DeliveryStatusTemporarilyFailed);
    }
    iterator = mPendingDeliveryReportUnknown;
    while(iterator.hasNext()) {
        iterator.next();
        sendDeliveryReport(iterator.key(), iterator.value(), Tp::DeliveryStatusUnknown);
    }

    mPendingDeliveryReportTemporarilyFailed.clear();
    mPendingDeliveryReportPermanentlyFailed.clear();
    mPendingDeliveryReportDelivered.clear();
    mPendingDeliveryReportAccepted.clear();
    mPendingDeliveryReportUnknown.clear();
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
    header["message-received"] = QDBusVariant(QDateTime::currentDateTime().toTime_t());
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
    header["message-received"] = QDBusVariant(QDateTime::currentDateTimeUtc().toTime_t());
    header["message-type"] = QDBusVariant(Tp::DeliveryStatusDelivered);
    header["x-canonical-mms"] = QDBusVariant(true);
    if (!subject.isEmpty())
    {
        header["subject"] = QDBusVariant(subject);
    }
    message << header;
    IncomingAttachmentList mmsdAttachments = qdbus_cast<IncomingAttachmentList>(properties["Attachments"]);
    Q_FOREACH(const IncomingAttachmentStruct &attachment, mmsdAttachments) {
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
