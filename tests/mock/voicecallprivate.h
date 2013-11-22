#ifndef VOICECALLPRIVATE_H
#define VOICECALLPRIVATE_H

#include <QDBusContext>

class VoiceCallPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.VoiceCall")
public:
    VoiceCallPrivate(const QString &path, QObject *parent = 0);
    ~VoiceCallPrivate();
Q_SIGNALS:
    void PropertyChanged(const QString &name, const QDBusVariant &value);
public Q_SLOTS:
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
    void Hangup();
    void Answer();
    void MockSetAlerting();
    void MockAnswer();
private:
    QVariantMap mProperties;
};

extern QMap<QString, VoiceCallPrivate*> voiceCallData;
extern QMap<QString, QVariantMap> initialCallProperties;

#endif
