#ifndef OFONOTEXTCHANNEL_H
#define OFONOTEXTCHANNEL_H

#include <QObject>

#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/Types>

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

private Q_SLOTS:
    void onOfonoMessageStateChanged(QString status);

private:
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
