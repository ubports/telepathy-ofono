/**
 * Copyright (C) 2016 Canonical, Ltd.
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
 * Authors: Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
 */

#ifndef MMSGROUPCACHE_H
#define MMSGROUPCACHE_H

#include <QObject>

typedef struct {
    QString groupId;
    QString subject;
    QStringList members;
} MMSGroup;

class MMSGroupCache : public QObject
{
    Q_OBJECT
public:
    static MMSGroup existingGroup(const QStringList &members);
    static MMSGroup existingGroup(const QString &groupId);
    static bool saveGroup(const MMSGroup &group);

private:
    explicit MMSGroupCache(QObject *parent = 0);
};

#endif // MMSGROUPCACHE_H
