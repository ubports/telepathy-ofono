#ifndef VOICECALLMANAGERPRIVATE_H
#define VOICECALLMANAGERPRIVATE_H

#include <QDBusContext>
#include <QDBusObjectPath>
#include "mock_common.h"

class OfonoVoiceCall;

class VoiceCallManagerPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.VoiceCallManager")
public:
    VoiceCallManagerPrivate(QObject *parent = 0);
    ~VoiceCallManagerPrivate();
Q_SIGNALS:
    void CallRemoved(const QDBusObjectPath &obj);
    void CallAdded(const QDBusObjectPath &obj, const QVariantMap &value);
    void PropertyChanged(const QString &name, const QDBusVariant &value);
    void TonesReceived(const QString &tones);
private Q_SLOTS:
    void onVoiceCallDestroyed();
public Q_SLOTS:
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
    QDBusObjectPath MockIncomingCall(const QString &from);
    QDBusObjectPath Dial(const QString &to, const QString &hideCallerId);
    QMap<QDBusObjectPath, QVariantMap> GetCalls();
    void SwapCalls();
    void SendTones(const QString &tones);
private:
    QVariantMap mProperties;
    QMap<QString, OfonoVoiceCall*> mVoiceCalls;
    int voiceCallCount;
};

extern QMap<QString, VoiceCallManagerPrivate*> voiceCallManagerData;

#endif
