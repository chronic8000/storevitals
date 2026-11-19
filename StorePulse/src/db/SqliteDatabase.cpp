#include "SqliteDatabase.hpp"

#include "MigrationRunner.hpp"
#include "Logger.hpp"

#include <QSqlError>
#include <QSqlQuery>

namespace storepulse {

SqliteDatabase::~SqliteDatabase()
{
    close();
}

bool SqliteDatabase::open(const QString& filePath)
{
    close();
    m_filePath = filePath;
    m_lastError.clear();

    if (QSqlDatabase::contains(QLatin1String(kConnectionName))) {
        QSqlDatabase::removeDatabase(QLatin1String(kConnectionName));
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                QLatin1String(kConnectionName));
    db.setDatabaseName(filePath);

    if (!db.open()) {
        m_lastError = db.lastError().text();
        Logger::error(QStringLiteral("Failed to open SQLite DB: %1").arg(m_lastError));
        return false;
    }

    m_opened = true;

    if (!configurePragmas()) {
        close();
        return false;
    }

    MigrationRunner migrator(db);
    if (!migrator.applyEmbeddedMigrations()) {
        m_lastError = migrator.lastError();
        Logger::error(QStringLiteral("Migration failed: %1").arg(m_lastError));
        close();
        return false;
    }

    Logger::info(QStringLiteral("SQLite opened in WAL mode at %1").arg(filePath));
    return true;
}

void SqliteDatabase::close()
{
    if (QSqlDatabase::contains(QLatin1String(kConnectionName))) {
        {
            QSqlDatabase db = QSqlDatabase::database(QLatin1String(kConnectionName));
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(QLatin1String(kConnectionName));
    }
    m_opened = false;
}

bool SqliteDatabase::isOpen() const
{
    return m_opened && QSqlDatabase::database(QLatin1String(kConnectionName)).isOpen();
}

QSqlDatabase SqliteDatabase::database() const
{
    return QSqlDatabase::database(QLatin1String(kConnectionName));
}

QString SqliteDatabase::lastError() const
{
    return m_lastError;
}

QString SqliteDatabase::filePath() const
{
    return m_filePath;
}

bool SqliteDatabase::configurePragmas()
{
    QSqlDatabase db = database();
    QSqlQuery q(db);

    const char* pragmas[] = {
        "PRAGMA journal_mode = WAL;",
        "PRAGMA synchronous = NORMAL;",
        "PRAGMA foreign_keys = ON;",
        "PRAGMA temp_store = MEMORY;",
        "PRAGMA busy_timeout = 5000;",
    };

    for (const char* sql : pragmas) {
        if (!q.exec(QLatin1String(sql))) {
            m_lastError = q.lastError().text();
            Logger::error(QStringLiteral("PRAGMA failed (%1): %2").arg(QLatin1String(sql), m_lastError));
            return false;
        }
    }

    // Confirm WAL actually engaged.
    if (!q.exec(QStringLiteral("PRAGMA journal_mode;")) || !q.next()) {
        m_lastError = QStringLiteral("Unable to read journal_mode");
        return false;
    }
    const QString mode = q.value(0).toString().toLower();
    if (mode != QLatin1String("wal")) {
        m_lastError = QStringLiteral("Expected WAL journal_mode, got '%1'").arg(mode);
        Logger::error(m_lastError);
        return false;
    }
    return true;
}

bool SqliteDatabase::beginTransaction()
{
    if (!database().transaction()) {
        m_lastError = database().lastError().text();
        return false;
    }
    return true;
}

bool SqliteDatabase::commit()
{
    if (!database().commit()) {
        m_lastError = database().lastError().text();
        return false;
    }
    return true;
}

bool SqliteDatabase::rollback()
{
    if (!database().rollback()) {
        m_lastError = database().lastError().text();
        return false;
    }
    return true;
}

} // namespace storepulse
