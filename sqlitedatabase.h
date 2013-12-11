/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * Authors:
 *  Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
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

#ifndef SQLITEDATABASE_H
#define SQLITEDATABASE_H

#include <QObject>
#include <QSqlDatabase>

class SQLiteDatabase : public QObject
{
    Q_OBJECT
public:
    static SQLiteDatabase *instance();

    bool initializeDatabase();
    QSqlDatabase database() const;

    bool beginTransation();
    bool finishTransaction();
    bool rollbackTransaction();

    bool reopen();

protected:
    bool createOrUpdateDatabase();
    QStringList parseSchemaFile(const QString &fileName);
    void parseVersionInfo();

private:
    explicit SQLiteDatabase(QObject *parent = 0);
    QString mDatabasePath;
    QSqlDatabase mDatabase;
    int mSchemaVersion;
    
};

#endif // SQLITEDATABASE_H
