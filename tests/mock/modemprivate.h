#ifndef MODEMPRIVATE_H
#define MODEMPRIVATE_H

#include <QDBusAbstractAdaptor>
#include "ofonomodem.h"

class OfonoVoiceCall;
class OfonoMessage;
class OfonoNetworkRegistration;

class ModemPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.canonical.OfonoQtMock.OfonoModem")
public:
    ModemPrivate(OfonoModem* modemIface, QObject *parent = 0);
    ~ModemPrivate();
    OfonoModem *ofonoModem() {
        return mOfonoModem;
    }
public Q_SLOTS:
    void setOnline(bool online);
private:
    OfonoModem *mOfonoModem;
    QMap <QString, OfonoVoiceCall*> mCalls;
    QMap <QString, OfonoMessage*> mMessages;

};

static QMap<QString, ModemPrivate*> modemData;

#endif
