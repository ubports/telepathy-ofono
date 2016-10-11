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

#include "mmsgroupcache.h"
#include "phoneutils_p.h"
#include "sqlitedatabase.h"
#include <QSqlQuery>
#include <QVariant>
#include <QCryptographicHash>

MMSGroupCache::MMSGroupCache(QObject *parent) : QObject(parent)
{
}

MMSGroup MMSGroupCache::existingGroup(const QStringList &members)
{
    MMSGroup group;
    if (members.isEmpty()) {
        return group;
    }

    // try to find all the threads which at least the first member is part of
    QString firstMember = members.first();
    QSqlQuery query(SQLiteDatabase::instance()->database());
    query.prepare("SELECT groupId FROM mms_group_members WHERE comparePhoneNumbers(memberId, :memberId)");
    query.bindValue(":memberId", firstMember);
    if (!query.exec()) {
        return group;
    }

    QStringList groupIds;
    while (query.next()) {
        groupIds << query.value(0).toString();
    }

    // now get all the groups and see if any matches all the members
    for (auto groupId : groupIds) {
        query.prepare("SELECT memberId FROM mms_group_members WHERE groupId=:groupId");
        query.bindValue(":groupId", groupId);
        if (!query.exec()) {
            return group;
        }
        QStringList groupMembers;
        while (query.next()) {
            groupMembers << query.value(0).toString();
        }

        // if the groups have a different number of members, they are certainly not the same
        if (groupMembers.count() != members.count()) {
            continue;
        }

        // compare the members to see if they match
        int match = 0;
        for (auto groupMember : groupMembers) {
            for (auto member : members) {
                if (PhoneUtils::comparePhoneNumbers(groupMember, member)) {
                    match++;
                    continue;
                }
            }
        }
        if (match == members.count()) {
            query.prepare("SELECT subject FROM mms_groups WHERE groupId=:groupId");
            query.bindValue(":groupId", groupId);
            if (query.exec()) {
                query.next();
                group.subject = query.value(0).toString();
            }
            group.groupId = groupId;
            group.members = groupMembers;
            break;
        }
    }
    return group;
}

MMSGroup MMSGroupCache::existingGroup(const QString &groupId)
{
    MMSGroup group;

    // select the group to make sure it exists
    QSqlQuery query(SQLiteDatabase::instance()->database());
    query.prepare("SELECT subject FROM mms_groups WHERE groupId=:groupId");
    query.bindValue(":groupId", groupId);
    if (query.exec() && query.next()) {
        group.groupId = groupId;
        group.subject = query.value(0).toString();
        query.prepare("SELECT memberId FROM mms_group_members WHERE groupId=:groupId");
        query.bindValue(":groupId", groupId);
        if (!query.exec()) {
            return group;
        }

        while (query.next()) {
            group.members << query.value(0).toString();
        }
    }

    return group;
}

bool MMSGroupCache::saveGroup(const MMSGroup &group)
{
    SQLiteDatabase::instance()->beginTransation();

    QSqlQuery query(SQLiteDatabase::instance()->database());
    query.prepare("INSERT INTO mms_groups(groupId, subject) VALUES (:groupId, :subject)");
    query.bindValue(":groupId", group.groupId);
    query.bindValue(":subject", group.subject);
    if (!query.exec()) {
        SQLiteDatabase::instance()->rollbackTransaction();
        return false;
    }
    for (auto member : group.members) {
        query.prepare("INSERT INTO mms_group_members(groupId, memberId) VALUES(:groupId, :memberId)");
        query.bindValue(":groupId", group.groupId);
        query.bindValue(":memberId", member);
        if (!query.exec()) {
            SQLiteDatabase::instance()->rollbackTransaction();
            return false;
        }
    }
    SQLiteDatabase::instance()->finishTransaction();
    return true;
}

QString MMSGroupCache::generateId(const QStringList &phoneNumbers)
{
    return QString(QCryptographicHash::hash(phoneNumbers.join(";").toLocal8Bit(),QCryptographicHash::Md5).toHex());
}
