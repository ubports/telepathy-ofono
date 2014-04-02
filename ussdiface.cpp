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

#include "ussdiface.h"

// Conn.I.USSD
BaseConnectionUSSDInterface::Adaptee::Adaptee(BaseConnectionUSSDInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}


struct TP_QT_NO_EXPORT BaseConnectionUSSDInterface::Private {
    Private(BaseConnectionUSSDInterface *parent)
        : adaptee(new BaseConnectionUSSDInterface::Adaptee(parent)) {
    }
    QString state;
    QString serial;
    InitiateCallback initiateCB;
    RespondCallback respondCB;
    CancelCallback cancelCB;
    BaseConnectionUSSDInterface::Adaptee *adaptee;
};

BaseConnectionUSSDInterface::Adaptee::~Adaptee()
{
}

void BaseConnectionUSSDInterface::Adaptee::initiate(const QString &command, const ConnectionInterfaceUSSDAdaptor::InitiateContextPtr &context)
{
    if (!mInterface->mPriv->initiateCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    Tp::DBusError error;
    mInterface->mPriv->initiateCB(command, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseConnectionUSSDInterface::Adaptee::respond(const QString &reply, const ConnectionInterfaceUSSDAdaptor::RespondContextPtr &context)
{
    if (!mInterface->mPriv->respondCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    Tp::DBusError error;
    mInterface->mPriv->respondCB(reply, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseConnectionUSSDInterface::Adaptee::cancel(const ConnectionInterfaceUSSDAdaptor::CancelContextPtr &context)
{
    if (!mInterface->mPriv->cancelCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    Tp::DBusError error;
    mInterface->mPriv->cancelCB(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

BaseConnectionUSSDInterface::BaseConnectionUSSDInterface()
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_USSD),
      mPriv(new Private(this))
{
}

BaseConnectionUSSDInterface::~BaseConnectionUSSDInterface()
{
    delete mPriv;
}

void BaseConnectionUSSDInterface::setInitiateCallback(const InitiateCallback &cb)
{
    mPriv->initiateCB = cb;
}

void BaseConnectionUSSDInterface::setRespondCallback(const RespondCallback &cb)
{
    mPriv->respondCB = cb;
}

void BaseConnectionUSSDInterface::setCancelCallback(const CancelCallback &cb)
{
    mPriv->cancelCB = cb;
}

QString BaseConnectionUSSDInterface::state() const
{
    return mPriv->state;
}

void BaseConnectionUSSDInterface::setSerial(const QString &serial) const
{
    mPriv->serial = serial;
}


QString BaseConnectionUSSDInterface::serial() const
{
    return mPriv->serial;
}

void BaseConnectionUSSDInterface::StateChanged(const QString &state)
{
    mPriv->state = state;
    Q_EMIT mPriv->adaptee->stateChanged(state);
}

void BaseConnectionUSSDInterface::InitiateUSSDComplete(const QString &ussdResp)
{
    Q_EMIT mPriv->adaptee->initiateUSSDComplete(ussdResp);
}

void BaseConnectionUSSDInterface::RespondComplete(bool success, const QString &ussdResp)
{
    Q_EMIT mPriv->adaptee->respondComplete(success, ussdResp);
}

void BaseConnectionUSSDInterface::BarringComplete(const QString &ssOp, const QString &cbService, const QVariantMap &cbMap)
{
    Q_EMIT mPriv->adaptee->barringComplete(ssOp, cbService, cbMap);
}

void BaseConnectionUSSDInterface::ForwardingComplete(const QString &ssOp, const QString &cfService, const QVariantMap &cfMap)
{
    Q_EMIT mPriv->adaptee->forwardingComplete(ssOp, cfService, cfMap);
}

void BaseConnectionUSSDInterface::WaitingComplete(const QString &ssOp, const QVariantMap &cwMap)
{
    Q_EMIT mPriv->adaptee->waitingComplete(ssOp, cwMap);
}

void BaseConnectionUSSDInterface::CallingLinePresentationComplete(const QString &ssOp, const QString &status)
{
    Q_EMIT mPriv->adaptee->callingLinePresentationComplete(ssOp, status);
}

void BaseConnectionUSSDInterface::ConnectedLinePresentationComplete(const QString &ssOp, const QString &status)
{
    Q_EMIT mPriv->adaptee->connectedLinePresentationComplete(ssOp, status);
}

void BaseConnectionUSSDInterface::CallingLineRestrictionComplete(const QString &ssOp, const QString &status)
{
    Q_EMIT mPriv->adaptee->callingLineRestrictionComplete(ssOp, status);
}

void BaseConnectionUSSDInterface::ConnectedLineRestrictionComplete(const QString &ssOp, const QString &status)
{
    Q_EMIT mPriv->adaptee->connectedLineRestrictionComplete(ssOp, status);
}

void BaseConnectionUSSDInterface::InitiateFailed()
{
    Q_EMIT mPriv->adaptee->initiateFailed();
}

void BaseConnectionUSSDInterface::NotificationReceived(const QString &message)
{
    Q_EMIT mPriv->adaptee->notificationReceived(message);
}

void BaseConnectionUSSDInterface::RequestReceived(const QString &message)
{
    Q_EMIT mPriv->adaptee->requestReceived(message);
}


QVariantMap BaseConnectionUSSDInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseConnectionUSSDInterface::createAdaptor()
{
    (void) new ConnectionInterfaceUSSDAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}


ConnectionInterfaceUSSDAdaptor::ConnectionInterfaceUSSDAdaptor(const QDBusConnection& bus, QObject* adaptee, QObject* parent)
    : Tp::AbstractAdaptor(bus, adaptee, parent)
{
    connect(adaptee, SIGNAL(notificationReceived(const QString &)), SIGNAL(NotificationReceived(const QString &)));
    connect(adaptee, SIGNAL(requestReceived(const QString &)), SIGNAL(RequestReceived(const QString &)));

    connect(adaptee, SIGNAL(initiateUSSDComplete(const QString &)), SIGNAL(InitiateUSSDComplete(const QString &)));

    connect(adaptee, SIGNAL(barringComplete(const QString &, const QString &, const QVariantMap &)), 
        SIGNAL(BarringComplete(const QString &, const QString &, const QVariantMap &)));

    connect(adaptee, SIGNAL(forwardingComplete(const QString &, const QString &, const QVariantMap &)), 
        SIGNAL(ForwardingComplete(const QString &, const QString &, const QVariantMap &)));

    connect(adaptee, SIGNAL(waitingComplete(const QString &, const QVariantMap &)), 
        SIGNAL(WaitingComplete(const QString &, const QVariantMap &)));

    connect(adaptee, SIGNAL(callingLinePresentationComplete(const QString &, const QString &)), 
        SIGNAL(CallingLinePresentationComplete(const QString &, const QString &)));

    connect(adaptee, SIGNAL(connectedLinePresentationComplete(const QString &, const QString &)), 
        SIGNAL(ConnectedLinePresentationComplete(const QString &, const QString &)));

    connect(adaptee, SIGNAL(callingLineRestrictionComplete(const QString &, const QString &)), 
        SIGNAL(CallingLineRestrictionComplete(const QString &, const QString &)));

    connect(adaptee, SIGNAL(connectedLineRestrictionComplete(const QString &, const QString &)), 
        SIGNAL(ConnectedLineRestrictionComplete(const QString &, const QString &)));

    connect(adaptee, SIGNAL(initiateFailed()), SIGNAL(InitiateFailed()));

    connect(adaptee, SIGNAL(stateChanged(const QString&)), SIGNAL(StateChanged(const QString&)));

    connect(adaptee, SIGNAL(respondComplete(bool, const QString &)), SIGNAL(RespondComplete(bool, const QString &)));
}

ConnectionInterfaceUSSDAdaptor::~ConnectionInterfaceUSSDAdaptor()
{
}

void ConnectionInterfaceUSSDAdaptor::Initiate(const QString &command, const QDBusMessage& dbusMessage)
{
    if (!adaptee()->metaObject()->indexOfMethod("initiate(const QString &,ConnectionInterfaceUSSDAdaptor::InitiateContextPtr)") == -1) {
        dbusConnection().send(dbusMessage.createErrorReply(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented")));
        return;
    }

    InitiateContextPtr ctx = InitiateContextPtr(
            new Tp::MethodInvocationContext< >(dbusConnection(), dbusMessage));
    QMetaObject::invokeMethod(adaptee(), "initiate",
        Q_ARG(QString, command),
        Q_ARG(ConnectionInterfaceUSSDAdaptor::InitiateContextPtr, ctx));
    return;
}

void ConnectionInterfaceUSSDAdaptor::Respond(const QString &reply, const QDBusMessage& dbusMessage)
{
    if (!adaptee()->metaObject()->indexOfMethod("respond(QConnectionInterfaceUSSDAdaptor::RespondContextPtr)") == -1) {
        dbusConnection().send(dbusMessage.createErrorReply(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented")));
        return;
    }

    RespondContextPtr ctx = RespondContextPtr(
            new Tp::MethodInvocationContext< >(dbusConnection(), dbusMessage));
    QMetaObject::invokeMethod(adaptee(), "respond",
        Q_ARG(QString, reply),
        Q_ARG(ConnectionInterfaceUSSDAdaptor::RespondContextPtr, ctx));
    return;
}

void ConnectionInterfaceUSSDAdaptor::Cancel(const QDBusMessage& dbusMessage)
{
    if (!adaptee()->metaObject()->indexOfMethod("cancel(ConnectionInterfaceUSSDAdaptor::CancelContextPtr)") == -1) {
        dbusConnection().send(dbusMessage.createErrorReply(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented")));
        return;
    }

    CancelContextPtr ctx = CancelContextPtr(
            new Tp::MethodInvocationContext< >(dbusConnection(), dbusMessage));
    QMetaObject::invokeMethod(adaptee(), "cancel",
        Q_ARG(ConnectionInterfaceUSSDAdaptor::CancelContextPtr, ctx));
    return;
}

QString ConnectionInterfaceUSSDAdaptor::Serial() const
{
    return qvariant_cast< QString >(adaptee()->property("serial"));
}


QString ConnectionInterfaceUSSDAdaptor::State() const
{
    return qvariant_cast< QString >(adaptee()->property("state"));
}

