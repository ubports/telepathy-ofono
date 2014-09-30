/****************************************************************************
**
** Copyright (C) 2014 Canonical, Ltd.
**
** Authors:
**  Andreas Pokorny <andreas.pokorny@canonical.com>
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
****************************************************************************/

#ifndef POWERDAUDIOMODEMEDIATOR_H
#define POWERDAUDIOMODEMEDIATOR_H

#include "powerd.h"

#include <QString>
#include <fstream>
#include <memory>

class PowerD;
/*!
 * \brief PowerDAudioModeMediator is responsible for configuring proximity
 * handling of powerd during different call states and used audio outputs.
 * In General that mean enabling sreen blanking on proximity events, when
 * a call is active and neither a bluetooth headset nor the speakers are used.
 */
class PowerDAudioModeMediator
{
public:
    PowerDAudioModeMediator(PowerD &powerd);
    void audioModeChanged(const QString &mode);
    void audioOutputClosed();
private:
    void apply() const;
    PowerD &powerd;
    bool mProximityEnabled{false};
};

#endif
