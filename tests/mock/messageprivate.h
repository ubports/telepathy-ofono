#ifndef MESSAGEPRIVATE_H
#define MESSAGEPRIVATE_H

#include <QDBusContext>

class OfonoMessage;
class OfonoInterface;

class MessagePrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.Message")
public:
    MessagePrivate(OfonoMessage *iface, OfonoInterface *prop_iface, QObject *parent = 0);
    ~MessagePrivate();
    OfonoInterface *getPropertiesInterface() {
        return mOfonoInterface;
    }
public Q_SLOTS:
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
    void Cancel();
    void MockMarkFailed();
    void MockMarkSent();
private:
    QVariantMap mProperties;
    OfonoMessage *mOfonoMessage;
    OfonoInterface *mOfonoInterface;
};

static QMap<QString, MessagePrivate*> messageData;

#endif
