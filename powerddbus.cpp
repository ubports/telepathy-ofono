/**
 * Copyright (C) 2014 Canonical, Ltd.
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
 * Authors: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "powerddbus.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

PowerDDBus::PowerDDBus()
    : mPowerDIface{
        new QDBusInterface(
            "com.canonical.powerd",
            "/com/canonical/powerd",
            "com.canonical.powerd",
            QDBusConnection::systemBus())}
{
}

void PowerDDBus::enableProximityHandling()
{
    mPowerDIface->call("enableProximityHandling", "telepathy-ofono");
}

void PowerDDBus::disableProximityHandling()
{
    mPowerDIface->call("disableProximityHandling", "telepathy-ofono");
}
