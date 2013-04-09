// ofono-qt
#include <ofonomessage.h>

// telepathy-ofono
#include "ofonotextchannel.h"

oFonoTextChannel::oFonoTextChannel(oFonoConnection *conn, QString phoneNumber, uint targetHandle, QObject *parent):
    QObject(parent),
    mConnection(conn),
    mPhoneNumber(phoneNumber),
    mTargetHandle(targetHandle),
    mMessageCounter(1)
{
    Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(mConnection, TP_QT_IFACE_CHANNEL_TYPE_TEXT, targetHandle, Tp::HandleTypeContact);
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
    QObject::connect(mBaseChannel.data(), SIGNAL(closed()), this, SLOT(deleteLater()));
}

oFonoTextChannel::~oFonoTextChannel()
{
}

Tp::BaseChannelPtr oFonoTextChannel::getBaseChannel()
{
    return mBaseChannel;
}

QString oFonoTextChannel::sendMessage(const Tp::MessagePartList& message, uint flags, Tp::DBusError* error)
{
    Tp::MessagePart header = message.at(0);
    Tp::MessagePart body = message.at(1);

    QString objpath = mConnection->getMessageManager()->sendMessage(mPhoneNumber, body["content"].variant().toString()).path();
    if (objpath.isEmpty()) {
        qDebug() << mConnection->getMessageManager()->errorName() << mConnection->getMessageManager()->errorMessage();
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, mConnection->getMessageManager()->errorMessage());
    }

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
            delivery_status = Tp::DeliveryStatusDelivered;
            msg->deleteLater();
        } else if(status == "failed") {
            delivery_status = Tp::DeliveryStatusPermanentlyFailed;
            msg->deleteLater();
        } else if(status == "pending") {
            delivery_status = Tp::DeliveryStatusAccepted;
        } else {
            delivery_status = Tp::DeliveryStatusUnknown;
        }

        Tp::MessagePartList partList;
        Tp::MessagePart header;
        header["message-sender"] = QDBusVariant(mTargetHandle);
        header["message-sender-id"] = QDBusVariant(mPhoneNumber);
        header["message-type"] = QDBusVariant(Tp::ChannelTextMessageTypeDeliveryReport);
        header["delivery-status"] = QDBusVariant(delivery_status);
        header["delivery-token"] = QDBusVariant(msg->path());
        partList << header;
        mTextChannel->addReceivedMessage(partList);
    }
}

void oFonoTextChannel::messageReceived(const QString &message, const QVariantMap &info)
{
    Tp::MessagePartList partList;

    Tp::MessagePart body;
    body["content-type"] = QDBusVariant("text/plain");
    body["content"] = QDBusVariant(message);

    Tp::MessagePart header;
    header["message-token"] = QDBusVariant(mMessageCounter++);
    header["message-received"] = QDBusVariant(QDateTime::fromString(info["SentTime"].toString(), Qt::ISODate).toTime_t());
    header["message-sender"] = QDBusVariant(mTargetHandle);
    header["message-sender-id"] = QDBusVariant(mPhoneNumber);
    header["message-type"] = QDBusVariant(Tp::ChannelTextMessageTypeNormal);
    partList << header << body;

    mTextChannel->addReceivedMessage(partList);
}
