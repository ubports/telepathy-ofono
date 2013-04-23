#ifndef OFONOCONNECTION_H
#define OFONOCONNECTION_H

// telepathy-qt
#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/BaseChannel>

// ofono-qt
#include <ofonomodemmanager.h>
#include <ofonomessagemanager.h>
#include <ofonovoicecallmanager.h>
#include <ofonovoicecall.h>
#include <ofonocallvolume.h>

// telepathy-ofono
#include "ofonotextchannel.h"
#include "ofonocallchannel.h"


class oFonoConnection;
class oFonoTextChannel;
class oFonoCallChannel;

class oFonoConnection : public Tp::BaseConnection
{
    Q_OBJECT
    Q_DISABLE_COPY(oFonoConnection)
public:
    oFonoConnection(const QDBusConnection &dbusConnection,
                    const QString &cmName,
                    const QString &protocolName,
                    const QVariantMap &parameters);

    QStringList inspectHandles(uint handleType, const Tp::UIntList& handles, Tp::DBusError *error);
    Tp::UIntList requestHandles(uint handleType, const QStringList& identifiers, Tp::DBusError* error);
    Tp::BaseChannelPtr createChannel(const QString& channelType, uint targetHandleType,
                                     uint targetHandle, Tp::DBusError *error);
    void connect(Tp::DBusError *error);

    Tp::BaseConnectionRequestsInterfacePtr requestsIface;
    uint newHandle(const QString &identifier);

    OfonoMessageManager *messageManager();
    OfonoVoiceCallManager *voiceCallManager();
    OfonoCallVolume *callVolume();

    uint ensureHandle(const QString &phoneNumber);

    ~oFonoConnection();

private Q_SLOTS:
    void oFonoIncomingMessage(const QString &message, const QVariantMap &info);
    void oFonoCallAdded(const QString &call, const QVariantMap &properties);
    void onTextChannelClosed();
    void onCallChannelClosed();

private:
    QMap<uint, QString> mHandles;

    QMap<QString, oFonoTextChannel*> mTextChannels;
    QMap<QString, oFonoCallChannel*> mCallChannels;

    QStringList mModems;
    OfonoModemManager *mOfonoModemManager;
    OfonoMessageManager *mOfonoMessageManager;
    OfonoVoiceCallManager *mOfonoVoiceCallManager;
    OfonoCallVolume *mOfonoCallVolume;
    uint mHandleCount;
};

#endif
