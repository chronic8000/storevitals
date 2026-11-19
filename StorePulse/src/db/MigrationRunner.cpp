#include "MigrationRunner.hpp"

#include "Logger.hpp"

#include <QFile>
#include <QSqlError>
#include <QSqlQuery>

namespace storepulse {
namespace {

QString stripLineComments(const QString& sql)
{
    QString out;
    out.reserve(sql.size());
    const QStringList lines = sql.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        const int comment = line.indexOf(QLatin1String("--"));
        if (comment >= 0) {
            out += line.left(comment);
        } else {
            out += line;
        }
        out += QLatin1Char('\n');
    }
    return out;
}

} // namespace

MigrationRunner::MigrationRunner(QSqlDatabase db)
    : m_db(std::move(db))
{
}

QString MigrationRunner::lastError() const
{
    return m_lastError;
}

bool MigrationRunner::ensureMigrationsTable()
{
    QSqlQuery q(m_db);
    const bool ok = q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "  version INTEGER PRIMARY KEY,"
        "  applied_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),"
        "  name TEXT NOT NULL"
        ")"));
    if (!ok) {
        m_lastError = q.lastError().text();
    }
    return ok;
}

int MigrationRunner::currentVersion() const
{
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT COALESCE(MAX(version), 0) FROM schema_migrations")) || !q.next()) {
        return 0;
    }
    return q.value(0).toInt();
}

bool MigrationRunner::isApplied(int version) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT 1 FROM schema_migrations WHERE version = ?"));
    q.addBindValue(version);
    if (!q.exec() || !q.next()) {
        return false;
    }
    return true;
}

bool MigrationRunner::recordApplied(int version, const QString& name)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO schema_migrations(version, name) VALUES (?, ?)"));
    q.addBindValue(version);
    q.addBindValue(name);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool MigrationRunner::execScript(const QString& sql)
{
    const QString cleaned = stripLineComments(sql);
    const QStringList parts = cleaned.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (QString part : parts) {
        part = part.trimmed();
        if (part.isEmpty()) {
            continue;
        }

        QSqlQuery q(m_db);
        if (!q.exec(part)) {
            m_lastError = QStringLiteral("%1 — while executing: %2")
                              .arg(q.lastError().text(), part.left(160));
            return false;
        }
    }
    return true;
}

bool MigrationRunner::applyEmbeddedMigrations()
{
    m_lastError.clear();
    if (!ensureMigrationsTable()) {
        return false;
    }

    struct Migration {
        int version;
        const char* name;
        const char* resourcePath;
    };

    const Migration migrations[] = {
        {1, "001_initial", ":/storepulse/db/migrations/001_initial.sql"},
        {2, "002_analytics_dimensions", ":/storepulse/db/migrations/002_analytics_dimensions.sql"},
        {3, "003_partner_center_full", ":/storepulse/db/migrations/003_partner_center_full.sql"},
        {4, "004_channel_pageviews", ":/storepulse/db/migrations/004_channel_pageviews.sql"},
        {5, "005_insights_pageviews", ":/storepulse/db/migrations/005_insights_pageviews.sql"},
        {6, "006_revenue", ":/storepulse/db/migrations/006_revenue.sql"},
        {7, "007_health_reviews_channels", ":/storepulse/db/migrations/007_health_reviews_channels.sql"},
    };

    if (!m_db.transaction()) {
        m_lastError = m_db.lastError().text();
        return false;
    }

    for (const Migration& m : migrations) {
        if (isApplied(m.version)) {
            continue;
        }

        QFile file(QLatin1String(m.resourcePath));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_lastError = QStringLiteral("Missing migration resource %1").arg(QLatin1String(m.resourcePath));
            m_db.rollback();
            return false;
        }

        const QString sql = QString::fromUtf8(file.readAll());
        Logger::info(QStringLiteral("Applying migration %1 (%2)").arg(m.version).arg(QLatin1String(m.name)));

        if (!execScript(sql)) {
            m_db.rollback();
            return false;
        }
        if (!recordApplied(m.version, QLatin1String(m.name))) {
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        m_lastError = m_db.lastError().text();
        return false;
    }
    return true;
}

} // namespace storepulse
