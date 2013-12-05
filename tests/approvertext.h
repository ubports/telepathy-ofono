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

#ifndef APPROVER_H
#define APPROVER_H

#include <QMap>
#include <TelepathyQt/AbstractClientApprover>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ChannelDispatchOperation>

#define TELEPHONY_SERVICE_HANDLER TP_QT_IFACE_CLIENT + ".TpOfonoTestHandler"
#define TELEPHONY_SERVICE_APPROVER TP_QT_IFACE_CLIENT + ".TpOfonoTestApprover"

class Approver : public QObject, public Tp::AbstractClientApprover
{
    Q_OBJECT

public:
    Approver(QObject *parent = 0);
    ~Approver();

    Tp::ChannelClassSpecList channelFilters() const;

    void addDispatchOperation(const Tp::MethodInvocationContextPtr<> &context,
                              const Tp::ChannelDispatchOperationPtr &dispatchOperation);

private Q_SLOTS:
    void processChannels();

private:
    QList<Tp::ChannelDispatchOperationPtr> mDispatchOps;
};

#endif // APPROVER_H
