#ifndef NETWORKREGISTRATIONPRIVATE_H
#define NETWORKREGISTRATIONPRIVATE_H

#include <QDBusContext>
#include "mock_common.h"

class NetworkRegistrationPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.NetworkRegistration")
public:
    NetworkRegistrationPrivate(QObject *parent = 0);
    ~NetworkRegistrationPrivate();
Q_SIGNALS:
    void PropertyChanged(const QString &, const QDBusVariant &);
public Q_SLOTS:
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
private:
    QVariantMap mProperties;
};

extern QMap<QString, NetworkRegistrationPrivate*> networkRegistrationData;

#endif
