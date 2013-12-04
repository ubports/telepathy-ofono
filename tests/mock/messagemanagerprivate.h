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

#ifndef MESSAGEMANAGERPRIVATE_H
#define MESSAGEMANAGERPRIVATE_H

#include <QDBusContext>
#include <QDBusObjectPath>
#include "mock_common.h"

class OfonoMessage;

class MessageManagerPrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.MessageManager")
public:
    MessageManagerPrivate(QObject *parent = 0);
    ~MessageManagerPrivate();
Q_SIGNALS:
    void MessageRemoved(const QDBusObjectPath &obj);
    void PropertyChanged(const QString &name, const QDBusVariant &value);
    void MessageAdded(const QDBusObjectPath &obj, const QVariantMap &value);
    void IncomingMessage(const QString &text, const QVariantMap &info);
private Q_SLOTS:
    void onMessageDestroyed();
public Q_SLOTS:
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
    void MockSendMessage(const QString &from, const QString &text);
    QDBusObjectPath SendMessage(const QString &to, const QString &text);
private:
    QVariantMap mProperties;
    QMap<QString, OfonoMessage*> mMessages;
    int messageCount;
};

extern QMap<QString, MessageManagerPrivate*> messageManagerData;

#endif
