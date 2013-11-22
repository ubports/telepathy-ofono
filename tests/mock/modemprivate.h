#ifndef MODEMPRIVATE_H
#define MODEMPRIVATE_H

#include <QDBusAbstractAdaptor>
#include "ofonomodem.h"
#include <QDBusVariant>

class OfonoVoiceCall;
class OfonoMessage;
class OfonoNetworkRegistration;

#define OFONO_MOCK_MODEM_OBJECT "/OfonoModem"

class ModemPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.Modem")
public:
    ModemPrivate(OfonoModem* modemIface, QObject *parent = 0);
    ~ModemPrivate();
    OfonoModem *ofonoModem() {
        return mOfonoModem;
    }
Q_SIGNALS:
    void PropertyChanged(const QString &name, const QDBusVariant& value);
public Q_SLOTS:
    void MockSetOnline(bool online);
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant& value);
private:
    QVariantMap mProperties;
    OfonoModem *mOfonoModem;
    QMap <QString, OfonoVoiceCall*> mCalls;
    QMap <QString, OfonoMessage*> mMessages;

};

extern QMap<QString, ModemPrivate*> modemData;

#endif
