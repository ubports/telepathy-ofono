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

#include <QDebug>

#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusObject>

#include "audiooutputsiface.h"


QDBusArgument &operator<<(QDBusArgument &argument, const AudioOutput &output)
{
    argument.beginStructure();
    argument << output.id << output.type << output.name;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, AudioOutput &output)
{
    argument.beginStructure();
    argument >> output.id >> output.type >> output.name;
    argument.endStructure();
    return argument;
}

// Chan.I.AudioOutputs
BaseChannelAudioOutputsInterface::Adaptee::Adaptee(BaseChannelAudioOutputsInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
    qDBusRegisterMetaType<AudioOutput>();
    qDBusRegisterMetaType<AudioOutputList>();
}

struct TP_QT_NO_EXPORT BaseChannelAudioOutputsInterface::Private {
    Private(BaseChannelAudioOutputsInterface *parent)
        : adaptee(new BaseChannelAudioOutputsInterface::Adaptee(parent)) {
    }
    AudioOutputList audioOutputs;
    QString activeAudioOutput;
    SetActiveAudioOutputCallback setActiveAudioOutputCB;
    BaseChannelAudioOutputsInterface::Adaptee *adaptee;
};

BaseChannelAudioOutputsInterface::Adaptee::~Adaptee()
{
}

void BaseChannelAudioOutputsInterface::Adaptee::setActiveAudioOutput(const QString &id, const ChannelInterfaceAudioOutputsAdaptor::SetActiveAudioOutputContextPtr &context)
{
    if (!mInterface->mPriv->setActiveAudioOutputCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    Tp::DBusError error;
    mInterface->mPriv->setActiveAudioOutputCB(id, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

BaseChannelAudioOutputsInterface::BaseChannelAudioOutputsInterface()
    : AbstractChannelInterface(TP_QT_IFACE_CHANNEL_AUDIOOUTPUTS),
      mPriv(new Private(this))
{
}

BaseChannelAudioOutputsInterface::~BaseChannelAudioOutputsInterface()
{
    delete mPriv;
}

QString BaseChannelAudioOutputsInterface::activeAudioOutput() const
{
    return mPriv->activeAudioOutput;
}

AudioOutputList BaseChannelAudioOutputsInterface::audioOutputs() const
{
    return mPriv->audioOutputs;
}

void BaseChannelAudioOutputsInterface::setSetActiveAudioOutputCallback(const SetActiveAudioOutputCallback &cb)
{
    mPriv->setActiveAudioOutputCB = cb;
}

void BaseChannelAudioOutputsInterface::setActiveAudioOutput(const QString &id)
{
    mPriv->activeAudioOutput = id;
    Q_EMIT mPriv->adaptee->activeAudioOutputChanged(id);
}

void BaseChannelAudioOutputsInterface::setAudioOutputs(const AudioOutputList &outputs)
{
    mPriv->audioOutputs = outputs;
    Q_EMIT mPriv->adaptee->audioOutputsChanged(outputs);
}

QVariantMap BaseChannelAudioOutputsInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseChannelAudioOutputsInterface::createAdaptor()
{
    (void) new ChannelInterfaceAudioOutputsAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}


ChannelInterfaceAudioOutputsAdaptor::ChannelInterfaceAudioOutputsAdaptor(const QDBusConnection& bus, QObject* adaptee, QObject* parent)
    : Tp::AbstractAdaptor(bus, adaptee, parent)
{
    connect(adaptee, SIGNAL(audioOutputsChanged(AudioOutputList)), SIGNAL(AudioOutputsChanged(AudioOutputList)));
    connect(adaptee, SIGNAL(activeAudioOutputChanged(QString)), SIGNAL(ActiveAudioOutputChanged(QString)));
}

ChannelInterfaceAudioOutputsAdaptor::~ChannelInterfaceAudioOutputsAdaptor()
{
}

void ChannelInterfaceAudioOutputsAdaptor::SetActiveAudioOutput(const QString &id, const QDBusMessage& dbusMessage)
{
    if (!adaptee()->metaObject()->indexOfMethod("setActiveAudioOutput(QString,ChannelInterfaceAudioOutputsAdaptor::SetActiveAudioOutputContextPtr)") == -1) {
        dbusConnection().send(dbusMessage.createErrorReply(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented")));
        return;
    }

    SetActiveAudioOutputContextPtr ctx = SetActiveAudioOutputContextPtr(
            new Tp::MethodInvocationContext< >(dbusConnection(), dbusMessage));
    QMetaObject::invokeMethod(adaptee(), "setActiveAudioOutput",
        Q_ARG(QString, id),
        Q_ARG(ChannelInterfaceAudioOutputsAdaptor::SetActiveAudioOutputContextPtr, ctx));
    return;
}

QString ChannelInterfaceAudioOutputsAdaptor::ActiveAudioOutput() const
{
    return qvariant_cast< QString >(adaptee()->property("activeAudioOutput"));
}

AudioOutputList ChannelInterfaceAudioOutputsAdaptor::AudioOutputs() const
{
    return qvariant_cast< AudioOutputList >(adaptee()->property("audioOutputs"));
}

