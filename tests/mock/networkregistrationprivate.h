#ifndef NETWORKREGISTRATIONPRIVATE_H
#define NETWORKREGISTRATIONPRIVATE_H

#include <QDBusContext>
#define OFONO_MOCK_NETWORK_REGISTRATION_OBJECT "/OfonoNetworkRegistration"

class OfonoNetworkRegistration;
class OfonoInterface;

class NetworkRegistrationPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.NetworkRegistration")
public:
    NetworkRegistrationPrivate(OfonoNetworkRegistration *iface, OfonoInterface *prop_iface, QObject *parent = 0);
    ~NetworkRegistrationPrivate();
Q_SIGNALS:
    void PropertyChanged(const QString &, const QDBusVariant &);
public Q_SLOTS:
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
private:
    QVariantMap mProperties;
    OfonoNetworkRegistration *mOfonoNetworkRegistration;
    OfonoInterface *mOfonoInterface;
};

static QMap<QString, NetworkRegistrationPrivate*> networkRegistrationData;

#endif
