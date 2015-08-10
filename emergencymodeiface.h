/**
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Tiago Salem Herrmann <tiago.herrmann@canonical.com>
 *          Gustavo Pichorim Boiko <gustavo.boiko@gmail.com>
 */

#ifndef OFONOEMERGENCYMODEIFACE_H
#define OFONOEMERGENCYMODEIFACE_H

// telepathy-qt
#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/AbstractAdaptor>
#include <TelepathyQt/DBusError>
#include <TelepathyQt/Callbacks>

class BaseConnectionEmergencyModeInterface;

typedef Tp::SharedPtr<BaseConnectionEmergencyModeInterface> BaseConnectionEmergencyModeInterfacePtr;

#define TP_QT_IFACE_CONNECTION_EMERGENCYMODE "com.canonical.Telephony.EmergencyMode"

class TP_QT_EXPORT BaseConnectionEmergencyModeInterface : public Tp::AbstractConnectionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionEmergencyModeInterface)

public:
    static BaseConnectionEmergencyModeInterfacePtr create() {
        return BaseConnectionEmergencyModeInterfacePtr(new BaseConnectionEmergencyModeInterface());
    }
    template<typename BaseConnectionEmergencyModeInterfaceSubclass>
    static Tp::SharedPtr<BaseConnectionEmergencyModeInterfaceSubclass> create() {
        return Tp::SharedPtr<BaseConnectionEmergencyModeInterfaceSubclass>(
                   new BaseConnectionEmergencyModeInterfaceSubclass());
    }
    QVariantMap immutableProperties() const;
    virtual ~BaseConnectionEmergencyModeInterface();

    typedef Tp::Callback1<QStringList, Tp::DBusError*> EmergencyNumbersCallback;
    void setEmergencyNumbersCallback(const EmergencyNumbersCallback &cb);
    void setFakeEmergencyNumber(const QString &fakeEmergencyNumber);

public Q_SLOTS:
    void setEmergencyNumbers(const QStringList &numbers);
    void setCountryCode(const QString &countryCode);

protected:
    BaseConnectionEmergencyModeInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};


class TP_QT_EXPORT ConnectionInterfaceEmergencyModeAdaptor : public Tp::AbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", TP_QT_IFACE_CONNECTION_EMERGENCYMODE)
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"com.canonical.Telephony.EmergencyMode\">\n"
"    <method name=\"EmergencyNumbers\">\n"
"      <arg direction=\"out\" type=\"as\" name=\"emergencyNumbers\"/>\n"
"    </method>\n"
"    <signal name=\"EmergencyNumbersChanged\">\n"
"      <arg type=\"as\" name=\"numbers\"/>\n"
"    </signal>\n"
"    <method name=\"CountryCode\">\n"
"      <arg direction=\"out\" type=\"s\" name=\"countryCode\"/>\n"
"    </method>\n"
"    <signal name=\"CountryCodeChanged\">\n"
"      <arg type=\"s\" name=\"countryCode\"/>\n"
"    </signal>\n"
"  </interface>\n"
"")

public:
    ConnectionInterfaceEmergencyModeAdaptor(const QDBusConnection& dbusConnection, QObject* adaptee, QObject* parent);
    virtual ~ConnectionInterfaceEmergencyModeAdaptor();

    typedef Tp::MethodInvocationContextPtr< QStringList > EmergencyNumbersContextPtr;
    typedef Tp::MethodInvocationContextPtr< QString > CountryCodeContextPtr;

public Q_SLOTS: // METHODS
    QStringList EmergencyNumbers(const QDBusMessage& dbusMessage);
    QString CountryCode(const QDBusMessage& dbusMessage);

Q_SIGNALS: // SIGNALS
    void EmergencyNumbersChanged(const QStringList &numbers);
    void CountryCodeChanged(const QString &countryCode);
};


class TP_QT_NO_EXPORT BaseConnectionEmergencyModeInterface::Adaptee : public QObject
{
    Q_OBJECT

public:
    Adaptee(BaseConnectionEmergencyModeInterface *interface);
    ~Adaptee();

private Q_SLOTS:
    void emergencyNumbers(const ConnectionInterfaceEmergencyModeAdaptor::EmergencyNumbersContextPtr &context);
    void countryCode(const ConnectionInterfaceEmergencyModeAdaptor::CountryCodeContextPtr &context);

Q_SIGNALS:
    void emergencyNumbersChanged(const QStringList &numbers);
    void countryCodeChanged(const QString &countryCode);

public:
    BaseConnectionEmergencyModeInterface *mInterface;
};

#endif
