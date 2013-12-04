/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * Authors:
 *  Tiago Salem Herrmann <tiago.herrmann@canonical.com>
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

#ifndef MESSAGEPRIVATE_H
#define MESSAGEPRIVATE_H

#include <QDBusContext>

class MessagePrivate : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ofono.Message")
public:
    MessagePrivate(const QString &path, QObject *parent = 0);
    ~MessagePrivate();
Q_SIGNALS:
    void PropertyChanged(const QString &name, const QDBusVariant &value);
public Q_SLOTS:
    QVariantMap GetProperties();
    void SetProperty(const QString &name, const QDBusVariant &value);
    void Cancel();
    void MockMarkFailed();
    void MockMarkSent();
private:
    QVariantMap mProperties;
};

extern QMap<QString, MessagePrivate*> messageData;

#endif
