#ifndef OFONOCALLCHANNEL_H
#define OFONOCALLCHANNEL_H

#include <QObject>

#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/BaseCall>
#include <TelepathyQt/Types>

#include <ofono-qt/ofonovoicecall.h>

#include "connection.h"

class oFonoConnection;

class oFonoCallChannel : public OfonoVoiceCall
{
    Q_OBJECT
public:
    oFonoCallChannel(oFonoConnection *conn, QString phoneNumber, uint targetHandle, QString voiceObj, QObject *parent = 0);
    ~oFonoCallChannel();
    Tp::BaseChannelPtr baseChannel();

    void onHangup(uint reason, const QString &detailedReason, const QString &message, Tp::DBusError* error);
    void onAccept(Tp::DBusError*);
    void onMuteStateChanged(const Tp::LocalMuteState &state, Tp::DBusError *error);
    void onHoldStateChanged(const Tp::LocalHoldState &state, const Tp::LocalHoldStateReason &reason, Tp::DBusError *error);

private Q_SLOTS:
    void oFonoCallStateChanged(const QString &state);
    void init();


    void onOfonoMuteChanged(bool mute);

private:
    QString mObjPath;
    Tp::BaseChannelPtr mBaseChannel;
    QString mPhoneNumber;
    oFonoConnection *mConnection;
    uint mTargetHandle;
    Tp::BaseChannelHoldInterfacePtr mHoldIface;
    Tp::BaseCallMuteInterfacePtr mMuteIface;
    Tp::BaseChannelCallTypePtr mCallChannel;
    Tp::BaseCallContentPtr mCallContent;
};

#endif // OFONOCALLCHANNEL_H
