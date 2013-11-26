/*
 * Copyright (C) 2013 Canonical, Ltd.
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

#include "ofonomockcontroller.h"
#include "mock/mock_common.h"

OfonoMockController::OfonoMockController(QObject *parent) :
    QObject(parent),
    mNetworkRegistrationInterface("org.ofono", OFONO_MOCK_NETWORK_REGISTRATION_OBJECT, "org.ofono.NetworkRegistration")
{
}

OfonoMockController *OfonoMockController::instance()
{
    static OfonoMockController *self = new OfonoMockController();
    return self;
}

void OfonoMockController::setNetworkRegistrationStatus(const QString &status)
{
    mNetworkRegistrationInterface.call("SetProperty", "Status", QVariant::fromValue(QDBusVariant(status)));
}
