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

#include <QDebug>

#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusObject>

#include "emergencymodeiface.h"

BaseConnectionEmergencyModeInterface::Adaptee::Adaptee(BaseConnectionEmergencyModeInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}


struct TP_QT_NO_EXPORT BaseConnectionEmergencyModeInterface::Private {
    Private(BaseConnectionEmergencyModeInterface *parent)
        : adaptee(new BaseConnectionEmergencyModeInterface::Adaptee(parent)) {
    }
    EmergencyNumbersCallback emergencyNumbersCB;
    BaseConnectionEmergencyModeInterface::Adaptee *adaptee;
    QString fakeEmergencyNumber;
};

BaseConnectionEmergencyModeInterface::Adaptee::~Adaptee()
{
}

void BaseConnectionEmergencyModeInterface::Adaptee::emergencyNumbers(const ConnectionInterfaceEmergencyModeAdaptor::EmergencyNumbersContextPtr &context)
{
    if (!mInterface->mPriv->emergencyNumbersCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }
    Tp::DBusError error;
    QStringList numbers = mInterface->mPriv->emergencyNumbersCB(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    if (mInterface->mPriv->fakeEmergencyNumber.isEmpty()) {
        context->setFinished(numbers);
    } else {
        context->setFinished(QStringList() << numbers << mInterface->mPriv->fakeEmergencyNumber);
    }
}

BaseConnectionEmergencyModeInterface::BaseConnectionEmergencyModeInterface()
    : AbstractConnectionInterface(TP_QT_IFACE_CONNECTION_EMERGENCYMODE),
      mPriv(new Private(this))
{
}

BaseConnectionEmergencyModeInterface::~BaseConnectionEmergencyModeInterface()
{
    delete mPriv;
}

void BaseConnectionEmergencyModeInterface::setEmergencyNumbersCallback(const EmergencyNumbersCallback &cb)
{
    mPriv->emergencyNumbersCB = cb;
}

void BaseConnectionEmergencyModeInterface::setEmergencyNumbers(const QStringList &numbers)
{
    QStringList finalEmergencyList(numbers);

    if (!mPriv->fakeEmergencyNumber.isEmpty()) {
        finalEmergencyList << mPriv->fakeEmergencyNumber;
    }
    
    Q_EMIT mPriv->adaptee->emergencyNumbersChanged(finalEmergencyList);
}

void BaseConnectionEmergencyModeInterface::setFakeEmergencyNumber(const QString &fakeEmergencyNumber)
{
    mPriv->fakeEmergencyNumber = fakeEmergencyNumber;
}

QVariantMap BaseConnectionEmergencyModeInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseConnectionEmergencyModeInterface::createAdaptor()
{
    (void) new ConnectionInterfaceEmergencyModeAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}


ConnectionInterfaceEmergencyModeAdaptor::ConnectionInterfaceEmergencyModeAdaptor(const QDBusConnection& bus, QObject* adaptee, QObject* parent)
    : Tp::AbstractAdaptor(bus, adaptee, parent)
{
    connect(adaptee, SIGNAL(emergencyNumbersChanged(QStringList)), SIGNAL(EmergencyNumbersChanged(QStringList)));
}

ConnectionInterfaceEmergencyModeAdaptor::~ConnectionInterfaceEmergencyModeAdaptor()
{
}

QStringList ConnectionInterfaceEmergencyModeAdaptor::EmergencyNumbers(const QDBusMessage& dbusMessage)
{
    if (!adaptee()->metaObject()->indexOfMethod("emergencyNumbers(ConnectionInterfaceEmergencyModeAdaptor::EmergencyNumbersContextPtr)") == -1) {
        dbusConnection().send(dbusMessage.createErrorReply(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented")));
        return QStringList();
    }

    EmergencyNumbersContextPtr ctx = EmergencyNumbersContextPtr(
            new Tp::MethodInvocationContext< QStringList >(dbusConnection(), dbusMessage));
    QMetaObject::invokeMethod(adaptee(), "emergencyNumbers",
        Q_ARG(ConnectionInterfaceEmergencyModeAdaptor::EmergencyNumbersContextPtr, ctx));
    return QStringList();
}
