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

#ifndef MMSDMESSAGE_H
#define MMSDMESSAGE_H

#include <QObject>
#include <QVariantMap>
#include <QDBusObjectPath>
#include <QStringList>

class MMSDMessage : public QObject
{
    Q_OBJECT
public:
    MMSDMessage(QString objectPath, QVariantMap properties, QObject *parent=0);
    ~MMSDMessage();

    QVariantMap properties() const;
    QString path() const;
    void markRead() const;
    // it should be called delete, but it is a reserved keyword in c++
    void remove() const;

Q_SIGNALS:
    void propertyChanged(const QString&, const QVariant&);

private Q_SLOTS:
    void onPropertyChanged(const QString&, const QVariant&);

private:
    QVariantMap m_properties;
    QString m_messagePath;
};

#endif
