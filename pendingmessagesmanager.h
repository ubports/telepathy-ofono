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

#include <QDateTime>
#include <QSqlDatabase>

struct PendingMessage
{
    QString recipientId;
    QDateTime timestamp;
};


class PendingMessagesManager : public QObject
{
    Q_OBJECT
public:
    static PendingMessagesManager *instance();

    void addPendingMessage(const QString &objectPath, const QString &id);
    void removePendingMessage(const QString &objectPath);
    QString recipientIdForMessageId(const QString &messageId);
private:
    explicit PendingMessagesManager(QObject *parent = 0);
    QSqlDatabase mDatabase;
};
