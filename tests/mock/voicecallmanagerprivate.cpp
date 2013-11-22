
#include <QDBusConnection>

#include "voicecallmanagerprivateadaptor.h"
#include "ofonovoicecallmanager.h"
#include "ofonovoicecall.h"
#include "ofonointerface.h"
#include "voicecallprivate.h"

VoiceCallManagerPrivate::VoiceCallManagerPrivate(OfonoVoiceCallManager *iface, OfonoInterface *prop_iface, QObject *parent) :
    mOfonoVoiceCallManager(iface),
    mOfonoInterface(prop_iface),
    voiceCallCount(0),
    QObject(parent)
{
    QDBusConnection::sessionBus().registerObject(OFONO_MOCK_VOICECALL_MANAGER_OBJECT, this);
    QDBusConnection::sessionBus().registerService("org.ofono");
    SetProperty("EmergencyNumbers", QDBusVariant(QVariant(QStringList())));
    new VoiceCallManagerAdaptor(this);
}

VoiceCallManagerPrivate::~VoiceCallManagerPrivate()
{
}

QMap<QDBusObjectPath, QVariantMap> VoiceCallManagerPrivate::GetCalls()
{
    QMap<QDBusObjectPath, QVariantMap> props;
    qDebug() << mVoiceCalls;
    Q_FOREACH(const QString &key, mVoiceCalls.keys()) {
        props[QDBusObjectPath(key)] = QVariantMap();
    }
    return props;
}

QVariantMap VoiceCallManagerPrivate::GetProperties()
{
    return mProperties;
}

void VoiceCallManagerPrivate::SetProperty(const QString &name, const QDBusVariant& value)
{
    qDebug() << "VoiceCallManagerPrivate::SetProperty" << name << value.variant();
    mProperties[name] = value.variant();
    Q_EMIT PropertyChanged(name, value);
}

QDBusObjectPath VoiceCallManagerPrivate::MockIncomingCall(const QString &from)
{
    qDebug() << "VoiceCallManagerPrivate::MockIncomingCall" << from;
    QString newPath("/OfonoVoiceCall"+QString::number(++voiceCallCount));
    QDBusObjectPath newPathObj(newPath);

    initialCallProperties["State"] = "incoming";
    initialCallProperties["LineIdentification"] = from;
    initialCallProperties["Name"] = "";
    initialCallProperties["Multiparty"] = false;
    initialCallProperties["RemoteHeld"] = false;
    initialCallProperties["RemoteMultiparty"] = false;
    initialCallProperties["Emergency"] = false;

    mVoiceCalls[newPath] = new OfonoVoiceCall(newPath);
    connect(mVoiceCalls[newPath], SIGNAL(destroyed()), this, SLOT(onVoiceCallDestroyed()));

    Q_EMIT CallAdded(newPathObj, initialCallProperties);

    return newPathObj;
}

QDBusObjectPath VoiceCallManagerPrivate::Dial(const QString &to, const QString &hideCallerId)
{
    qDebug() << "DIAL" << to;
    QString newPath("/OfonoVoiceCall"+QString::number(++voiceCallCount));
    QDBusObjectPath newPathObj(newPath);

    initialCallProperties["State"] = "dialing";
    initialCallProperties["LineIdentification"] = to;
    initialCallProperties["Name"] = "";
    initialCallProperties["Multiparty"] = false;
    initialCallProperties["RemoteHeld"] = false;
    initialCallProperties["RemoteMultiparty"] = false;
    initialCallProperties["Emergency"] = false;

    mVoiceCalls[newPath] = new OfonoVoiceCall(newPath);
    connect(mVoiceCalls[newPath], SIGNAL(destroyed()), this, SLOT(onVoiceCallDestroyed()));

    Q_EMIT CallAdded(newPathObj, initialCallProperties);

    return newPathObj;
}

void VoiceCallManagerPrivate::onVoiceCallDestroyed()
{
    OfonoVoiceCall *voiceCall = static_cast<OfonoVoiceCall*>(sender());
    if (voiceCall) {
        mVoiceCalls.remove(voiceCall->path());
        Q_EMIT CallRemoved(QDBusObjectPath(voiceCall->path()));
    }
}
