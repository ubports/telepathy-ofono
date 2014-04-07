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
 */

#ifndef OFONOUSSDIFACE_H
#define OFONOUSSDIFACE_H

// telepathy-qt
#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/AbstractAdaptor>
#include <TelepathyQt/DBusError>
#include <TelepathyQt/Callbacks>

class BaseConnectionUSSDInterface;

typedef Tp::SharedPtr<BaseConnectionUSSDInterface> BaseConnectionUSSDInterfacePtr;

#define TP_QT_IFACE_CONNECTION_USSD "com.canonical.Telephony.USSD"

class TP_QT_EXPORT BaseConnectionUSSDInterface : public Tp::AbstractConnectionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseConnectionUSSDInterface)

public:
    static BaseConnectionUSSDInterfacePtr create() {
        return BaseConnectionUSSDInterfacePtr(new BaseConnectionUSSDInterface());
    }
    template<typename BaseConnectionUSSDInterfaceSubclass>
    static Tp::SharedPtr<BaseConnectionUSSDInterfaceSubclass> create() {
        return Tp::SharedPtr<BaseConnectionUSSDInterfaceSubclass>(
                   new BaseConnectionUSSDInterfaceSubclass());
    }
    QVariantMap immutableProperties() const;
    virtual ~BaseConnectionUSSDInterface();

    typedef Tp::Callback2<void, const QString&, Tp::DBusError*> InitiateCallback;
    void setInitiateCallback(const InitiateCallback &cb);

    typedef Tp::Callback2<void, const QString&, Tp::DBusError*> RespondCallback;
    void setRespondCallback(const RespondCallback &cb);

    typedef Tp::Callback1<void, Tp::DBusError*> CancelCallback;
    void setCancelCallback(const CancelCallback &cb);


    QString state() const;
    QString serial() const;
    void setSerial(const QString& serial) const;

public Q_SLOTS:
    void NotificationReceived(const QString &message);
    void RequestReceived(const QString &message);

    void InitiateUSSDComplete(const QString &ussdResp);
    void RespondComplete(bool success, const QString &ussdResp);
    void BarringComplete(const QString &ssOp, const QString &cbService, const QVariantMap &cbMap);
    void ForwardingComplete(const QString &ssOp, const QString &cfService, const QVariantMap &cfMap);
    void WaitingComplete(const QString &ssOp, const QVariantMap &cwMap);
    void CallingLinePresentationComplete(const QString &ssOp, const QString &status);
    void ConnectedLinePresentationComplete(const QString &ssOp, const QString &status);
    void CallingLineRestrictionComplete(const QString &ssOp, const QString &status);
    void ConnectedLineRestrictionComplete(const QString &ssOp, const QString &status);
    void InitiateFailed();
    void StateChanged(const QString &state);

protected:
    BaseConnectionUSSDInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};


class TP_QT_EXPORT ConnectionInterfaceUSSDAdaptor : public Tp::AbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", TP_QT_IFACE_CONNECTION_USSD)
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"com.canonical.Telephony.USSD\">\n"
"    <property access=\"read\" type=\"s\" name=\"State\"/>\n"
"    <property access=\"read\" type=\"s\" name=\"Serial\"/>\n"
"    <method name=\"Initiate\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"command\"/>\n"
"    </method>\n"
"    <method name=\"Respond\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"reply\"/>\n"
"    </method>\n"
"    <method name=\"Cancel\" />\n"
"    <signal name=\"NotificationReceived\">\n"
"      <arg type=\"s\" name=\"message\"/>\n"
"    </signal>\n"
"    <signal name=\"RequestReceived\">\n"
"      <arg type=\"s\" name=\"message\"/>\n"
"    </signal>\n"
"    <signal name=\"InitiateUSSDComplete\">\n"
"      <arg type=\"s\" name=\"response\"/>\n"
"    </signal>\n"
"    <signal name=\"RespondComplete\">\n"
"      <arg type=\"b\" name=\"success\"/>\n"
"      <arg type=\"s\" name=\"response\"/>\n"
"    </signal>\n"
"    <signal name=\"BarringComplete\">\n"
"      <arg type=\"s\" name=\"ssOp\"/>\n"
"      <arg type=\"s\" name=\"cbService\"/>\n"
"      <arg type=\"a{sv}\" name=\"cbMap\"/>\n"
"    </signal>\n"
"    <signal name=\"ForwardingComplete\">\n"
"      <arg type=\"s\" name=\"ssOp\"/>\n"
"      <arg type=\"s\" name=\"cfService\"/>\n"
"      <arg type=\"a{sv}\" name=\"cfMap\"/>\n"
"    </signal>\n"
"    <signal name=\"WaitingComplete\">\n"
"      <arg type=\"s\" name=\"ssOp\"/>\n"
"      <arg type=\"a{sv}\" name=\"cwMap\"/>\n"
"    </signal>\n"
"    <signal name=\"CallingLinePresentationComplete\">\n"
"      <arg type=\"s\" name=\"ssOp\"/>\n"
"      <arg type=\"s\" name=\"status\"/>\n"
"    </signal>\n"
"    <signal name=\"ConnectedLinePresentationComplete\">\n"
"      <arg type=\"s\" name=\"ssOp\"/>\n"
"      <arg type=\"s\" name=\"status\"/>\n"
"    </signal>\n"
"    <signal name=\"CallingLineRestrictionComplete\">\n"
"      <arg type=\"s\" name=\"ssOp\"/>\n"
"      <arg type=\"s\" name=\"status\"/>\n"
"    </signal>\n"
"    <signal name=\"ConnectedLineRestrictionComplete\">\n"
"      <arg type=\"s\" name=\"ssOp\"/>\n"
"      <arg type=\"s\" name=\"status\"/>\n"
"    </signal>\n"
"    <signal name=\"InitiateFailed\" />\n"
"    <signal name=\"StateChanged\">\n"
"      <arg type=\"s\" name=\"state\"/>\n"
"    </signal>\n"
"  </interface>\n"
"")
    Q_PROPERTY(QString State READ State)
    Q_PROPERTY(QString Serial READ Serial)

public:
    ConnectionInterfaceUSSDAdaptor(const QDBusConnection& dbusConnection, QObject* adaptee, QObject* parent);
    virtual ~ConnectionInterfaceUSSDAdaptor();

    typedef Tp::MethodInvocationContextPtr< > InitiateContextPtr;
    typedef Tp::MethodInvocationContextPtr< > RespondContextPtr;
    typedef Tp::MethodInvocationContextPtr< > CancelContextPtr;

public Q_SLOTS: // METHODS
    void Initiate(const QString &command, const QDBusMessage& dbusMessage);
    void Respond(const QString &reply, const QDBusMessage& dbusMessage);
    void Cancel(const QDBusMessage& dbusMessage);

    QString State() const;
    QString Serial() const;

Q_SIGNALS: // SIGNALS
    void NotificationReceived(const QString &message);
    void RequestReceived(const QString &message);

    void InitiateUSSDComplete(const QString &ussdResp);
    void RespondComplete(bool success, const QString &ussdResp);
    void BarringComplete(const QString &ssOp, const QString &cbService, const QVariantMap &cbMap);
    void ForwardingComplete(const QString &ssOp, const QString &cfService, const QVariantMap &cfMap);
    void WaitingComplete(const QString &ssOp, const QVariantMap &cwMap);
    void CallingLinePresentationComplete(const QString &ssOp, const QString &status);
    void ConnectedLinePresentationComplete(const QString &ssOp, const QString &status);
    void CallingLineRestrictionComplete(const QString &ssOp, const QString &status);
    void ConnectedLineRestrictionComplete(const QString &ssOp, const QString &status);
    void InitiateFailed();

    void StateChanged(const QString &state);
};


class TP_QT_NO_EXPORT BaseConnectionUSSDInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString state READ state)
    Q_PROPERTY(QString serial READ serial)

public:
    Adaptee(BaseConnectionUSSDInterface *interface);
    ~Adaptee();
    QString state() const
    {
        return mInterface->state();
    }
    QString serial() const
    {
        return mInterface->serial();
    }


private Q_SLOTS:
    void initiate(const QString &command, const ConnectionInterfaceUSSDAdaptor::InitiateContextPtr &context);
    void respond(const QString &reply, const ConnectionInterfaceUSSDAdaptor::RespondContextPtr &context);
    void cancel(const ConnectionInterfaceUSSDAdaptor::CancelContextPtr &context);

Q_SIGNALS:
    void notificationReceived(const QString &message);
    void requestReceived(const QString &message);

    void initiateUSSDComplete(const QString &ussdResp);
    void barringComplete(const QString &ssOp, const QString &cbService, const QVariantMap &cbMap);
    void forwardingComplete(const QString &ssOp, const QString &cfService, const QVariantMap &cfMap);
    void waitingComplete(const QString &ssOp, const QVariantMap &cwMap);
    void callingLinePresentationComplete(const QString &ssOp, const QString &status);
    void connectedLinePresentationComplete(const QString &ssOp, const QString &status);
    void callingLineRestrictionComplete(const QString &ssOp, const QString &status);
    void connectedLineRestrictionComplete(const QString &ssOp, const QString &status);
    void initiateFailed();
    void respondComplete(bool success, const QString &response);

    void stateChanged(const QString &state);

public:
    BaseConnectionUSSDInterface *mInterface;
};

#endif
