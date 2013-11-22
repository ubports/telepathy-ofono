#ifndef VOICECALLMANAGERPRIVATE_H
#define VOICECALLMANAGERPRIVATE_H

#include <QDBusContext>
#include <QDBusObjectPath>

class OfonoVoiceCallManager;
class OfonoVoiceCall;
class OfonoInterface;

#define OFONO_MOCK_VOICECALL_MANAGER_OBJECT "/OfonoVoiceCallManager"

class VoiceCallManagerPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.VoiceCallManager")
public:
    VoiceCallManagerPrivate(OfonoVoiceCallManager *iface, OfonoInterface *prop_iface, QObject *parent = 0);
    ~VoiceCallManagerPrivate();
Q_SIGNALS:
    void CallRemoved(const QDBusObjectPath &obj);
    void CallAdded(const QDBusObjectPath &obj, const QVariantMap &value);
    void PropertyChanged(const QString &name, const QDBusVariant &value);
private Q_SLOTS:
    void onVoiceCallDestroyed();
public Q_SLOTS:
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
    QDBusObjectPath MockIncomingCall(const QString &from);
    QDBusObjectPath Dial(const QString &to, const QString &hideCallerId);
    QMap<QDBusObjectPath, QVariantMap> GetCalls();
private:
    QVariantMap mProperties;
    QMap<QString, OfonoVoiceCall*> mVoiceCalls;
    OfonoVoiceCallManager *mOfonoVoiceCallManager;
    OfonoInterface *mOfonoInterface;
    int voiceCallCount;
};

extern QMap<QString, VoiceCallManagerPrivate*> voiceCallManagerData;

#endif
