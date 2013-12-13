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
 * Authors: Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
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
