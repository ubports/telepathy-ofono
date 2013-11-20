#ifndef MESSAGEMANAGERPRIVATE_H
#define MESSAGEMANAGERPRIVATE_H

#include <QDBusContext>

class OfonoMessageManager;
class OfonoInterface;

class MessageManagerPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.MessageManager")
public:
    MessageManagerPrivate(OfonoMessageManager *iface, OfonoInterface *prop_iface, QObject *parent = 0);
    ~MessageManagerPrivate();
    OfonoInterface *getPropertiesInterface() {
        return mOfonoInterface;
    }
public Q_SLOTS:
    void setProperty(const QString &name, const QString &value);
    void sendMessage(const QString &from, const QString &text);
private:
    OfonoMessageManager *mOfonoMessageManager;
    OfonoInterface *mOfonoInterface;
};

static QMap<QString, MessageManagerPrivate*> messageManagerData;

#endif
