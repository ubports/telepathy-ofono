#ifndef MODEMPRIVATE_H
#define MODEMPRIVATE_H

#include <QDBusAbstractAdaptor>
#include "ofonomodem.h"

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
public Q_SLOTS:
    void setOnline(bool online);
    void setProperty(const QString &name, const QVariant& value);
private:
    OfonoModem *mOfonoModem;
    QMap <QString, OfonoVoiceCall*> mCalls;
    QMap <QString, OfonoMessage*> mMessages;

};

static QMap<QString, ModemPrivate*> modemData;

#endif
