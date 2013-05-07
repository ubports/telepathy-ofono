/**
 * Copyright (C) 2012-2013 Canonical, Ltd.
 *
 * Authors:
 *  Tiago Salem Herrmann <tiago.herrmann@canonical.com>
 *
 * This file is part of telepathy-ofono.
 *
 * telepathy-ofono is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * telepathy-ofono is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OFONOVOICEMAILIFACE_H
#define OFONOVOICEMAILIFACE_H

// telepathy-qt
#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/AbstractAdaptor>
#include <TelepathyQt/DBusError>
#include <TelepathyQt/Callbacks>

class BaseConnectionVoicemailInterface;

typedef Tp::SharedPtr<BaseConnectionVoicemailInterface> BaseConnectionVoicemailInterfacePtr;

class TP_QT_EXPORT BaseConnectionVoicemailInterface : public Tp::AbstractConnectionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionVoicemailInterface)

public:
    static BaseConnectionVoicemailInterfacePtr create() {
        return BaseConnectionVoicemailInterfacePtr(new BaseConnectionVoicemailInterface());
    }
    template<typename BaseConnectionVoicemailInterfaceSubclass>
    static Tp::SharedPtr<BaseConnectionVoicemailInterfaceSubclass> create() {
        return Tp::SharedPtr<BaseConnectionVoicemailInterfaceSubclass>(
                   new BaseConnectionVoicemailInterfaceSubclass());
    }
    QVariantMap immutableProperties() const;
    virtual ~BaseConnectionVoicemailInterface();

    typedef Tp::Callback1<uint, Tp::DBusError*> VoicemailCountCallback;
    void setVoicemailCountCallback(const VoicemailCountCallback &cb);

    typedef Tp::Callback1<bool, Tp::DBusError*> VoicemailIndicatorCallback;
    void setVoicemailIndicatorCallback(const VoicemailIndicatorCallback &cb);

    typedef Tp::Callback1<QString, Tp::DBusError*> VoicemailNumberCallback;
    void setVoicemailNumberCallback(const VoicemailNumberCallback &cb);

public Q_SLOTS:
    void setVoicemailCount(int count);
    void setVoicemailIndicator(bool active);

protected:
    BaseConnectionVoicemailInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};


class TP_QT_EXPORT ConnectionInterfaceVoicemailAdaptor : public Tp::AbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.canonical.Telephony")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"com.canonical.Telephony\">\n"
"    <method name=\"VoicemailIndicator\">\n"
"      <arg direction=\"out\" type=\"b\" name=\"voicemailIndicator\"/>\n"
"    </method>\n"
"    <method name=\"VoicemailNumber\">\n"
"      <arg direction=\"out\" type=\"s\" name=\"voicemailNumber\"/>\n"
"    </method>\n"
"    <method name=\"VoicemailCount\">\n"
"      <arg direction=\"out\" type=\"u\" name=\"voicemailCount\"/>\n"
"    </method>\n"
"    <signal name=\"VoicemailCountChanged\">\n"
"      <arg type=\"u\" name=\"count\"/>\n"
"    </signal>\n"
"    <signal name=\"VoicemailIndicatorChanged\">\n"
"      <arg type=\"b\" name=\"active\"/>\n"
"    </signal>\n"
"  </interface>\n"
"")

public:
    ConnectionInterfaceVoicemailAdaptor(const QDBusConnection& dbusConnection, QObject* adaptee, QObject* parent);
    virtual ~ConnectionInterfaceVoicemailAdaptor();

    typedef Tp::MethodInvocationContextPtr< bool > VoicemailIndicatorContextPtr;
    typedef Tp::MethodInvocationContextPtr< QString > VoicemailNumberContextPtr;
    typedef Tp::MethodInvocationContextPtr< uint > VoicemailCountContextPtr;

public Q_SLOTS: // METHODS
    bool VoicemailIndicator(const QDBusMessage& dbusMessage);
    QString VoicemailNumber(const QDBusMessage& dbusMessage);
    uint VoicemailCount(const QDBusMessage& dbusMessage);

Q_SIGNALS: // SIGNALS
    void VoicemailCountChanged(uint count);
    void VoicemailIndicatorChanged(bool active);
};


class TP_QT_NO_EXPORT BaseConnectionVoicemailInterface::Adaptee : public QObject
{
    Q_OBJECT

public:
    Adaptee(BaseConnectionVoicemailInterface *interface);
    ~Adaptee();

private Q_SLOTS:
    void voicemailIndicator(const ConnectionInterfaceVoicemailAdaptor::VoicemailIndicatorContextPtr &context);
    void voicemailNumber(const ConnectionInterfaceVoicemailAdaptor::VoicemailNumberContextPtr &context);
    void voicemailCount(const ConnectionInterfaceVoicemailAdaptor::VoicemailCountContextPtr &context);

Q_SIGNALS:
    void voicemailCountChanged(uint count);
    void voicemailIndicatorChanged(bool active);

public:
    BaseConnectionVoicemailInterface *mInterface;
};

#endif
