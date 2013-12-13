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

#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include "pendingmessagesmanager.h"
#include "sqlitedatabase.h"

PendingMessagesManager::PendingMessagesManager(QObject *parent) :
    QObject(parent),
    mDatabase(SQLiteDatabase::instance()->database())
{
    populatePendingMessages();
}

PendingMessagesManager *PendingMessagesManager::instance()
{
    static PendingMessagesManager *self = new PendingMessagesManager();
    return self;
}

QString PendingMessagesManager::recipientIdForMessageId(const QString &messageId)
{
    QSqlQuery query(SQLiteDatabase::instance()->database());
    QString queryString("SELECT recipientId FROM pending_messages WHERE messageId=:messageId");
    query.prepare(queryString);
    query.bindValue(":messageId", messageId);

    if (!query.exec()) {
        qCritical() << "Error:" << query.lastError() << query.lastQuery();
        return QString();
    }

    if(!query.next()) {
        return QString();
    }

    return query.value(0).toString();
}

void PendingMessagesManager::populatePendingMessages()
{
    QSqlQuery query(SQLiteDatabase::instance()->database());
    QString queryString("SELECT messageId,recipientId,timestamp FROM pending_messages");
    query.prepare(queryString);
    if (!query.exec()) {
        qCritical() << "Error:" << query.lastError() << query.lastQuery();
        return;
    }

    while (query.next()) {
        PendingMessage message;
        QString messageId = query.value(0).toString();
        message.recipientId = query.value(1).toString();
        message.timestamp = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);
        mPendingMessages[messageId] = message;
    }
}

void PendingMessagesManager::addPendingMessage(const QString &messageId, const QString &recipientId)
{
    QSqlQuery query(SQLiteDatabase::instance()->database());

    PendingMessage message;
    message.recipientId = recipientId;
    message.timestamp = QDateTime::currentDateTimeUtc();

    QString queryString("INSERT into pending_messages (messageId, recipientId, timestamp) VALUES (:messageId, :recipientId, :timestamp)");
    query.prepare(queryString);
    query.bindValue(":messageId", messageId);
    query.bindValue(":recipientId", message.recipientId);
    query.bindValue(":timestamp", message.timestamp.toString(Qt::ISODate));

    if (!query.exec()) {
        qCritical() << "Error:" << query.lastError() << query.lastQuery();
        return;
    }
    mPendingMessages[messageId] = message;
}

void PendingMessagesManager::removePendingMessage(const QString &messageId)
{
    QSqlQuery query(SQLiteDatabase::instance()->database());
    QString queryString("DELETE FROM pending_messages WHERE messageId=:messageId");

    query.prepare(queryString);
    query.bindValue(":messageId", messageId);

    if (!query.exec()) {
        qCritical() << "Error:" << query.lastError() << query.lastQuery();
        return;
    }

    mPendingMessages.remove(messageId);
}


