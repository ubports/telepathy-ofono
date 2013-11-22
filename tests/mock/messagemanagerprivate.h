#ifndef MESSAGEMANAGERPRIVATE_H
#define MESSAGEMANAGERPRIVATE_H

#include <QDBusContext>
#include <QDBusObjectPath>
#include "mock_common.h"

class OfonoMessage;

class MessageManagerPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.MessageManager")
public:
    MessageManagerPrivate(QObject *parent = 0);
    ~MessageManagerPrivate();
Q_SIGNALS:
    void MessageRemoved(const QDBusObjectPath &obj);
    void PropertyChanged(const QString &name, const QDBusVariant &value);
    void MessageAdded(const QDBusObjectPath &obj, const QVariantMap &value);
    void IncomingMessage(const QString &text, const QVariantMap &info);
private Q_SLOTS:
    void onMessageDestroyed();
public Q_SLOTS:
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
    void MockSendMessage(const QString &from, const QString &text);
    QDBusObjectPath SendMessage(const QString &to, const QString &text);
private:
    QVariantMap mProperties;
    QMap<QString, OfonoMessage*> mMessages;
    int messageCount;
};

extern QMap<QString, MessageManagerPrivate*> messageManagerData;

#endif
