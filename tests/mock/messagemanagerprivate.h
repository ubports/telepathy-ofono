#ifndef MESSAGEMANAGERPRIVATE_H
#define MESSAGEMANAGERPRIVATE_H

#include <QDBusContext>
#include <QDBusObjectPath>

class OfonoMessageManager;
class OfonoMessage;
class OfonoInterface;

#define OFONO_MOCK_MESSAGE_MANAGER_OBJECT "/OfonoMessageManager"

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
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
    void MockSendMessage(const QString &from, const QString &text);
    QDBusObjectPath SendMessage(const QString &to, const QString &text);
private:
    QVariantMap mProperties;
    QMap<QString, OfonoMessage*> mMessages;
    OfonoMessageManager *mOfonoMessageManager;
    OfonoInterface *mOfonoInterface;
    int messageCount;
};

static QMap<QString, MessageManagerPrivate*> messageManagerData;

#endif
