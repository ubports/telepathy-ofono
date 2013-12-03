#ifndef CALLVOLUMEPRIVATE_H
#define CALLVOLUMEPRIVATE_H

#include <QDBusContext>
#include <QDBusVariant>
#include "mock_common.h"

class CallVolumePrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.CallVolume")
public:
    CallVolumePrivate(QObject *parent = 0);
    ~CallVolumePrivate();
Q_SIGNALS:
    void PropertyChanged(const QString &name, const QDBusVariant &value);
public Q_SLOTS:
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
private:
    QVariantMap mProperties;
};

extern QMap<QString, CallVolumePrivate*> callVolumeData;

#endif
