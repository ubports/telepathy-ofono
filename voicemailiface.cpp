/**
 * Copyright (C) 2013 Canonical, Ltd.
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

#include <QDebug>

#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusObject>

#include "voicemailiface.h"

// Conn.I.Voicemail
BaseConnectionVoicemailInterface::Adaptee::Adaptee(BaseConnectionVoicemailInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}


struct TP_QT_NO_EXPORT BaseConnectionVoicemailInterface::Private {
    Private(BaseConnectionVoicemailInterface *parent)
        : adaptee(new BaseConnectionVoicemailInterface::Adaptee(parent)) {
    }
    VoicemailCountCallback voicemailCountCB;
    VoicemailNumberCallback voicemailNumberCB;
    VoicemailIndicatorCallback voicemailIndicatorCB;
    BaseConnectionVoicemailInterface::Adaptee *adaptee;
};

BaseConnectionVoicemailInterface::Adaptee::~Adaptee()
{
}

void BaseConnectionVoicemailInterface::Adaptee::voicemailIndicator(const ConnectionInterfaceVoicemailAdaptor::VoicemailIndicatorContextPtr &context)
{
    if (!mInterface->mPriv->voicemailIndicatorCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    Tp::DBusError error;
    bool active = mInterface->mPriv->voicemailIndicatorCB(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(active);
}

void BaseConnectionVoicemailInterface::Adaptee::voicemailNumber(const ConnectionInterfaceVoicemailAdaptor::VoicemailNumberContextPtr &context)
{
    if (!mInterface->mPriv->voicemailNumberCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    Tp::DBusError error;
    QString number = mInterface->mPriv->voicemailNumberCB(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(number);
}

void BaseConnectionVoicemailInterface::Adaptee::voicemailCount(const ConnectionInterfaceVoicemailAdaptor::VoicemailCountContextPtr &context)
{
    if (!mInterface->mPriv->voicemailCountCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    Tp::DBusError error;
    uint count = mInterface->mPriv->voicemailCountCB(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(count);
}


BaseConnectionVoicemailInterface::BaseConnectionVoicemailInterface()
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_VOICEMAIL),
      mPriv(new Private(this))
{
}

BaseConnectionVoicemailInterface::~BaseConnectionVoicemailInterface()
{
    delete mPriv;
}

void BaseConnectionVoicemailInterface::setVoicemailIndicatorCallback(const VoicemailIndicatorCallback &cb)
{
    mPriv->voicemailIndicatorCB = cb;
}

void BaseConnectionVoicemailInterface::setVoicemailNumberCallback(const VoicemailNumberCallback &cb)
{
    mPriv->voicemailNumberCB = cb;
}

void BaseConnectionVoicemailInterface::setVoicemailCountCallback(const VoicemailCountCallback &cb)
{
    mPriv->voicemailCountCB = cb;
}

void BaseConnectionVoicemailInterface::setVoicemailCount(int count)
{
    Q_EMIT mPriv->adaptee->voicemailCountChanged(uint(count));
}

void BaseConnectionVoicemailInterface::setVoicemailIndicator(bool active)
{
    Q_EMIT mPriv->adaptee->voicemailIndicatorChanged(active);
}

QVariantMap BaseConnectionVoicemailInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseConnectionVoicemailInterface::createAdaptor()
{
    (void) new ConnectionInterfaceVoicemailAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}


ConnectionInterfaceVoicemailAdaptor::ConnectionInterfaceVoicemailAdaptor(const QDBusConnection& bus, QObject* adaptee, QObject* parent)
    : Tp::AbstractAdaptor(bus, adaptee, parent)
{
    connect(adaptee, SIGNAL(voicemailCountChanged(uint)), SIGNAL(VoicemailCountChanged(uint)));
    connect(adaptee, SIGNAL(voicemailIndicatorChanged(bool)), SIGNAL(VoicemailIndicatorChanged(bool)));
}

ConnectionInterfaceVoicemailAdaptor::~ConnectionInterfaceVoicemailAdaptor()
{
}

bool ConnectionInterfaceVoicemailAdaptor::VoicemailIndicator(const QDBusMessage& dbusMessage)
{
    if (!adaptee()->metaObject()->indexOfMethod("voicemailIndicator(ConnectionInterfaceVoicemailAdaptor::VoicemailIndicatorContextPtr)") == -1) {
        dbusConnection().send(dbusMessage.createErrorReply(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented")));
        return bool();
    }

    VoicemailIndicatorContextPtr ctx = VoicemailIndicatorContextPtr(
            new Tp::MethodInvocationContext< bool >(dbusConnection(), dbusMessage));
    QMetaObject::invokeMethod(adaptee(), "voicemailIndicator",
        Q_ARG(ConnectionInterfaceVoicemailAdaptor::VoicemailIndicatorContextPtr, ctx));
    return bool();
}

QString ConnectionInterfaceVoicemailAdaptor::VoicemailNumber(const QDBusMessage& dbusMessage)
{
    if (!adaptee()->metaObject()->indexOfMethod("voicemailNumber(ConnectionInterfaceVoicemailAdaptor::VoicemailNumberContextPtr)") == -1) {
        dbusConnection().send(dbusMessage.createErrorReply(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented")));
        return QString();
    }

    VoicemailNumberContextPtr ctx = VoicemailNumberContextPtr(
            new Tp::MethodInvocationContext< QString >(dbusConnection(), dbusMessage));
    QMetaObject::invokeMethod(adaptee(), "voicemailNumber",
        Q_ARG(ConnectionInterfaceVoicemailAdaptor::VoicemailNumberContextPtr, ctx));
    return QString();
}

uint ConnectionInterfaceVoicemailAdaptor::VoicemailCount(const QDBusMessage& dbusMessage)
{
    if (!adaptee()->metaObject()->indexOfMethod("voicemailCount(ConnectionInterfaceVoicemailAdaptor::VoicemailCountContextPtr)") == -1) {
        dbusConnection().send(dbusMessage.createErrorReply(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented")));
        return uint();
    }

    VoicemailCountContextPtr ctx = VoicemailCountContextPtr(
            new Tp::MethodInvocationContext< uint >(dbusConnection(), dbusMessage));
    QMetaObject::invokeMethod(adaptee(), "voicemailCount",
        Q_ARG(ConnectionInterfaceVoicemailAdaptor::VoicemailCountContextPtr, ctx));
    return uint();
}

