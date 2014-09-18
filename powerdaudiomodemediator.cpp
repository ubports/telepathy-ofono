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

#include "powerdaudiomodemediator.h"

PowerDAudioModeMediator::PowerDAudioModeMediator(PowerD &powerd)
    : powerd(powerd)
{
}

void PowerDAudioModeMediator::audioModeChanged(const QString &mode)
{
    PowerD::PowerState expectedState =
        (mode == "speaker" || mode == "bluetooth") ?
        PowerD::ActiveDisplay :
        PowerD::ActiveDisplayWithProximityBlanking;

    if (!last_request || last_request->state != expectedState)
    {
        decltype(last_request) old_request{ std::move(last_request) };
        last_request.reset(new PendingRequest{ powerd.requestState(expectedState), expectedState });

        if (last_request->cookie.isEmpty())
            last_request.reset();

        if (old_request)
            powerd.clearState( old_request->cookie );
    }
}

void PowerDAudioModeMediator::channelHangup()
{
    if (last_request)
    {
        powerd.clearState( last_request->cookie );
        last_request.reset();
    }
}
