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

#ifndef MMSDSERVICE_H
#define MMSDSERVICE_H

#include <QObject>
#include <QVariantMap>
#include <QDBusObjectPath>
#include <QStringList>

#include "dbustypes.h"
#include "connection.h"
#include "mmsdmessage.h"

class MMSDService : public QObject
{
    Q_OBJECT
public:
    MMSDService(QString objectPath, oFonoConnection *connection, QObject *parent=0);
    ~MMSDService();

    QVariantMap properties() const;
    MessageList messages() const;
    QString path() const;

    QDBusObjectPath sendMessage(QStringList recipients, OutgoingAttachmentList attachments);

Q_SIGNALS:
    void messageAdded(const QString &messagePath, const QVariantMap &properties);
    void messageRemoved(const QString &messagePath);

private Q_SLOTS:
    void onMessageAdded(const QDBusObjectPath &path, const QVariantMap &properties);
    void onMessageRemoved(const QDBusObjectPath &path);

private:
    QVariantMap m_properties;
    QString m_servicePath;
    MessageList m_messages;
};

#endif
