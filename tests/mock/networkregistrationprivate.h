#ifndef NETWORKREGISTRATIONPRIVATE_H
#define NETWORKREGISTRATIONPRIVATE_H

#include <QDBusContext>

class OfonoNetworkRegistration;
class OfonoInterface;

class NetworkRegistrationPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.canonical.ofonoQtMock.NetworkRegistration")
public:
    NetworkRegistrationPrivate(OfonoNetworkRegistration *iface, OfonoInterface *prop_iface, QObject *parent = 0);
    ~NetworkRegistrationPrivate();
    OfonoInterface *getPropertiesInterface() {
        return mOfonoInterface;
    }
public Q_SLOTS:
    void setProperty(const QString &name, const QString &value);
private:
    OfonoNetworkRegistration *mOfonoNetworkRegistration;
    OfonoInterface *mOfonoInterface;
};

static QMap<QString, NetworkRegistrationPrivate*> networkRegistrationData;

#endif
