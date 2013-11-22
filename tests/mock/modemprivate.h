#ifndef MODEMPRIVATE_H
#define MODEMPRIVATE_H

#include <QDBusContext>
#include <QDBusAbstractAdaptor>
#include <QDBusVariant>

#define OFONO_MOCK_MODEM_OBJECT "/OfonoModem"

class ModemPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.Modem")
public:
    ModemPrivate(QObject *parent = 0);
    ~ModemPrivate();
Q_SIGNALS:
    void PropertyChanged(const QString &name, const QDBusVariant& value);
public Q_SLOTS:
    void MockSetOnline(bool online);
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant& value);
private:
    QVariantMap mProperties;
};

extern QMap<QString, ModemPrivate*> modemData;

#endif
