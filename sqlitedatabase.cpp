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

#include "phoneutils_p.h"
#include "sqlite3.h"
#include "sqlitedatabase.h"
#include <QStandardPaths>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QDir>

Q_DECLARE_OPAQUE_POINTER(sqlite3*)
Q_DECLARE_METATYPE(sqlite3*)

// custom sqlite function "comparePhoneNumbers" used to compare IDs if necessary
void comparePhoneNumbers(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    QString arg1((const char*)sqlite3_value_text(argv[0]));
    QString arg2((const char*)sqlite3_value_text(argv[1]));
    sqlite3_result_int(context, (int)PhoneUtils::comparePhoneNumbers(arg1, arg2));
}

SQLiteDatabase::SQLiteDatabase(QObject *parent) :
    QObject(parent), mSchemaVersion(0)
{
    initializeDatabase();
}

SQLiteDatabase *SQLiteDatabase::instance()
{
    static SQLiteDatabase *self = new SQLiteDatabase();
    return self;
}

bool SQLiteDatabase::initializeDatabase()
{
    mDatabasePath = qgetenv("TP_OFONO_SQLITE_DBPATH");

    if (mDatabasePath.isEmpty()) {
        mDatabasePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

        QDir dir(mDatabasePath);
        if (!dir.exists("telepathy-ofono") && !dir.mkpath("telepathy-ofono")) {
            qCritical() << "Failed to create dir";
            return false;
        }
        dir.cd("telepathy-ofono");

        mDatabasePath = dir.absoluteFilePath("telepathy-ofono.sqlite");
    }

    mDatabase = QSqlDatabase::addDatabase("QSQLITE");
    mDatabase.setDatabaseName(mDatabasePath);

    // always run the createDatabase function at least during the development
    if (!createOrUpdateDatabase()) {
        qCritical() << "Failed to create or update the database";
        return false;
    }

    return true;
}

QSqlDatabase SQLiteDatabase::database() const
{
    return mDatabase;
}

bool SQLiteDatabase::beginTransation()
{
    return mDatabase.transaction();
}

bool SQLiteDatabase::finishTransaction()
{
    return mDatabase.commit();
}

bool SQLiteDatabase::rollbackTransaction()
{
    return mDatabase.rollback();
}

/// this method is to be used mainly by unit tests in order to clean up the database between
/// tests.
bool SQLiteDatabase::reopen()
{
    mDatabase.close();
    mDatabase.open();

    // make sure the database is up-to-date after reopening.
    // this is mainly required for the memory backend used for testing
    createOrUpdateDatabase();
}

bool SQLiteDatabase::createOrUpdateDatabase()
{
    bool create = !QFile(mDatabasePath).exists();

    if (!mDatabase.open()) {
        return false;
    }

    // create the comparePhoneNumbers custom sqlite function
    sqlite3 *handle = database().driver()->handle().value<sqlite3*>();
    sqlite3_create_function(handle, "comparePhoneNumbers", 2, SQLITE_ANY, NULL, &comparePhoneNumbers, NULL, NULL);

    parseVersionInfo();

    QSqlQuery query(mDatabase);

    QStringList statements;

    if (create) {
         statements = parseSchemaFile(":/database/schema/schema.sql");
    } else {
        // if the database already exists, we don´t need to create the tables
        // only check if an update is needed
        query.exec("SELECT * FROM schema_version");
        if (!query.exec() || !query.next()) {
            return false;
        }

        int upgradeToVersion = query.value(0).toInt() + 1;
        while (upgradeToVersion <= mSchemaVersion) {
            statements += parseSchemaFile(QString(":/database/schema/v%1.sql").arg(QString::number(upgradeToVersion)));
            ++upgradeToVersion;
        }
    }

    // if at this point needsUpdate is still false, it means the database is up-to-date
    if (statements.isEmpty()) {
        return true;
    }

    beginTransation();

    Q_FOREACH(const QString &statement, statements) {
        if (!query.exec(statement)) {
            qCritical() << "Failed to create or update database. SQL Statements:" << query.lastQuery() << "Error:" << query.lastError();
            rollbackTransaction();
            return false;
        }
    }

    // now set the new database schema version
    if (!query.exec("DELETE FROM schema_version")) {
        qCritical() << "Failed to remove previous schema versions. SQL Statement:" << query.lastQuery() << "Error:" << query.lastError();
        rollbackTransaction();
        return false;
    }

    if (!query.exec(QString("INSERT INTO schema_version VALUES (%1)").arg(mSchemaVersion))) {
        qCritical() << "Failed to insert new schema version. SQL Statement:" << query.lastQuery() << "Error:" << query.lastError();
        rollbackTransaction();
        return false;
    }

    finishTransaction();

    return true;
}

QStringList SQLiteDatabase::parseSchemaFile(const QString &fileName)
{
    QFile schema(fileName);
    if (!schema.open(QFile::ReadOnly)) {
        qCritical() << "Failed to open " << fileName;
        return QStringList();
    }

    bool parsingBlock = false;
    QString statement;
    QStringList statements;

    // FIXME: this parser is very basic, it needs to be improved in the future
    //        it does a lot of assumptions based on the structure of the schema.sql file

    QTextStream stream(&schema);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        bool statementEnded = false;

        statement += line;

        // check if we are parsing a trigger command
        if (line.trimmed().startsWith("CREATE TRIGGER", Qt::CaseInsensitive)) {
            parsingBlock = true;
        } else if (parsingBlock) {
            if (line.contains("END;")) {
                parsingBlock = false;
                statementEnded = true;
            }
        } else if (statement.contains(";")) {
            statementEnded = true;
        }

        statement += "\n";

        if (statementEnded) {
            statements.append(statement);
            statement.clear();
        }
    }

    return statements;
}

void SQLiteDatabase::parseVersionInfo()
{
    QFile schema(":/database/schema/version.info");
    if (!schema.open(QFile::ReadOnly)) {
        qDebug() << schema.error();
        qCritical() << "Failed to get database version";
    }

    QString version = schema.readAll();
    mSchemaVersion = version.toInt();
}
