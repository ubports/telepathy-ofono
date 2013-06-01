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

#ifndef OFONOSPEAKERIFACE_H
#define OFONOSPEAKERIFACE_H

// telepathy-qt
#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/AbstractAdaptor>
#include <TelepathyQt/DBusError>
#include <TelepathyQt/Callbacks>

#define TP_QT_IFACE_CHANNEL_SPEAKER "com.canonical.Telephony.Speaker"

class BaseChannelSpeakerInterface;

typedef Tp::SharedPtr<BaseChannelSpeakerInterface> BaseChannelSpeakerInterfacePtr;

class TP_QT_EXPORT BaseChannelSpeakerInterface : public Tp::AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelSpeakerInterface)

public:
    static BaseChannelSpeakerInterfacePtr create() {
        return BaseChannelSpeakerInterfacePtr(new BaseChannelSpeakerInterface());
    }
    template<typename BaseChannelSpeakerInterfaceSubclass>
    static Tp::SharedPtr<BaseChannelSpeakerInterfaceSubclass> create() {
        return Tp::SharedPtr<BaseChannelSpeakerInterfaceSubclass>(
                   new BaseChannelSpeakerInterfaceSubclass());
    }
    QVariantMap immutableProperties() const;
    virtual ~BaseChannelSpeakerInterface();
    bool speakerMode() const;

    typedef Tp::Callback2<void, bool, Tp::DBusError*> turnOnSpeakerCallback;
    void setTurnOnSpeakerCallback(const turnOnSpeakerCallback &cb);

public Q_SLOTS:
    void setSpeakerMode(bool active);

protected:
    BaseChannelSpeakerInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};


class TP_QT_EXPORT ChannelInterfaceSpeakerAdaptor : public Tp::AbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", TP_QT_IFACE_CHANNEL_SPEAKER)
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"com.canonical.Telephony.Speaker\">\n"
"    <property access=\"read\" type=\"b\" name=\"SpeakerMode\"/>\n"
"    <method name=\"turnOnSpeaker\">\n"
"      <arg direction=\"in\" type=\"b\" name=\"active\"/>\n"
"    </method>\n"
"    <signal name=\"SpeakerChanged\">\n"
"      <arg type=\"b\" name=\"active\"/>\n"
"    </signal>\n"
"  </interface>\n"
"")
    Q_PROPERTY(bool SpeakerMode READ SpeakerMode)
public:
    ChannelInterfaceSpeakerAdaptor(const QDBusConnection& dbusConnection, QObject* adaptee, QObject* parent);
    virtual ~ChannelInterfaceSpeakerAdaptor();

    typedef Tp::MethodInvocationContextPtr< bool > turnOnSpeakerContextPtr;

public: // PROPERTIES
    bool SpeakerMode() const;

public Q_SLOTS: // METHODS
    void turnOnSpeaker(bool active, const QDBusMessage& dbusMessage);

Q_SIGNALS: // SIGNALS
    void SpeakerChanged(bool active);
};


class TP_QT_NO_EXPORT BaseChannelSpeakerInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool speakerMode READ speakerMode)
public:
    Adaptee(BaseChannelSpeakerInterface *interface);
    ~Adaptee();
    bool speakerMode() const
    {
        return mInterface->speakerMode();
    }

private Q_SLOTS:
    void turnOnSpeaker(bool active, const ChannelInterfaceSpeakerAdaptor::turnOnSpeakerContextPtr &context);

Q_SIGNALS:
    void speakerChanged(bool active);

public:
    BaseChannelSpeakerInterface *mInterface;
};

#endif
