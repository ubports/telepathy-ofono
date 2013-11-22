#ifndef MESSAGEPRIVATE_H
#define MESSAGEPRIVATE_H

#include <QDBusContext>

class MessagePrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.Message")
public:
    MessagePrivate(const QString &path, QObject *parent = 0);
    ~MessagePrivate();
Q_SIGNALS:
    void PropertyChanged(const QString &name, const QDBusVariant &value);
public Q_SLOTS:
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
    void Cancel();
    void MockMarkFailed();
    void MockMarkSent();
private:
    QVariantMap mProperties;
};

extern QMap<QString, MessagePrivate*> messageData;

#endif
