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

#ifndef OFONOAUDIOOUTPUTSIFACE_H
#define OFONOAUDIOOUTPUTSIFACE_H

// telepathy-qt
#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/AbstractAdaptor>
#include <TelepathyQt/DBusError>
#include <TelepathyQt/Callbacks>

#include "dbustypes.h"

#define TP_QT_IFACE_CHANNEL_AUDIOOUTPUTS "com.canonical.Telephony.AudioOutputs"

class BaseChannelAudioOutputsInterface;

typedef Tp::SharedPtr<BaseChannelAudioOutputsInterface> BaseChannelAudioOutputsInterfacePtr;

class TP_QT_EXPORT BaseChannelAudioOutputsInterface : public Tp::AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelAudioOutputsInterface)

public:
    static BaseChannelAudioOutputsInterfacePtr create() {
        return BaseChannelAudioOutputsInterfacePtr(new BaseChannelAudioOutputsInterface());
    }
    template<typename BaseChannelAudioOutputsInterfaceSubclass>
    static Tp::SharedPtr<BaseChannelAudioOutputsInterfaceSubclass> create() {
        return Tp::SharedPtr<BaseChannelAudioOutputsInterfaceSubclass>(
                   new BaseChannelAudioOutputsInterfaceSubclass());
    }
    QVariantMap immutableProperties() const;
    virtual ~BaseChannelAudioOutputsInterface();
    AudioOutputList audioOutputs() const;
    QString activeAudioOutput() const;

    typedef Tp::Callback2<void, QString, Tp::DBusError*> SetActiveAudioOutputCallback;
    void setSetActiveAudioOutputCallback(const SetActiveAudioOutputCallback &cb);

public Q_SLOTS:
    void setActiveAudioOutput(const QString &id);
    void setAudioOutputs(const AudioOutputList &outputs);

protected:
    BaseChannelAudioOutputsInterface();

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};


class TP_QT_EXPORT ChannelInterfaceAudioOutputsAdaptor : public Tp::AbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", TP_QT_IFACE_CHANNEL_AUDIOOUTPUTS)
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"com.canonical.Telephony.AudioOutputs\">\n"
"    <property access=\"read\" type=\"s\" name=\"ActiveAudioOutput\"/>\n"
"    <property access=\"read\" type=\"a(sss)\" name=\"AudioOutputs\"/>\n"
"    <method name=\"SetActiveAudioOutput\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"id\"/>\n"
"    </method>\n"
"    <signal name=\"AudioOutputsChanged\">\n"
"      <arg type=\"a(sss)\" name=\"outputs\"/>\n"
"    </signal>\n"
"    <signal name=\"ActiveAudioOutputChanged\">\n"
"      <arg type=\"s\" name=\"id\"/>\n"
"    </signal>\n"
"  </interface>\n"
"")
    Q_PROPERTY(AudioOutputList AudioOutputs READ AudioOutputs)
    Q_PROPERTY(QString ActiveAudioOutput READ ActiveAudioOutput)
public:
    ChannelInterfaceAudioOutputsAdaptor(const QDBusConnection& dbusConnection, QObject* adaptee, QObject* parent);
    virtual ~ChannelInterfaceAudioOutputsAdaptor();

    typedef Tp::MethodInvocationContextPtr< > SetActiveAudioOutputContextPtr;

public: // PROPERTIES
    QString ActiveAudioOutput() const;
    AudioOutputList AudioOutputs() const;

public Q_SLOTS: // METHODS
    void SetActiveAudioOutput(const QString &id, const QDBusMessage& dbusMessage);

Q_SIGNALS: // SIGNALS
    void AudioOutputsChanged(const AudioOutputList &outputs);
    void ActiveAudioOutputChanged(const QString &id);
};


class TP_QT_NO_EXPORT BaseChannelAudioOutputsInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AudioOutputList audioOutputs READ audioOutputs)
    Q_PROPERTY(QString activeAudioOutput READ activeAudioOutput)

public:
    Adaptee(BaseChannelAudioOutputsInterface *interface);
    ~Adaptee();
    AudioOutputList audioOutputs() const
    {
        return mInterface->audioOutputs();
    }

    QString activeAudioOutput() const
    {
        return mInterface->activeAudioOutput();
    }


private Q_SLOTS:
    void setActiveAudioOutput(const QString &id, const ChannelInterfaceAudioOutputsAdaptor::SetActiveAudioOutputContextPtr &context);

Q_SIGNALS:
    void activeAudioOutputChanged(const QString &id);
    void audioOutputsChanged(const AudioOutputList &outputs);

public:
    BaseChannelAudioOutputsInterface *mInterface;
};

#endif
