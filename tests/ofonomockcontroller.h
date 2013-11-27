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

#ifndef OFONOMOCKCONTROLLER_H
#define OFONOMOCKCONTROLLER_H

#include <QObject>
#include <QDBusInterface>

class OfonoMockController : public QObject
{
    Q_OBJECT
public:
    static OfonoMockController *instance();

Q_SIGNALS:

public Q_SLOTS:
    void NetworkRegistrationSetStatus(const QString &status);
    void MessageManagerSendMessage(const QString &from, const QString &text);

private:
    explicit OfonoMockController(QObject *parent = 0);
    QDBusInterface mNetworkRegistrationInterface;
    QDBusInterface mMessageManagerInterface;
};

#endif // OFONOMOCKCONTROLLER_H
