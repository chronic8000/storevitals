#include "FailureHitRepository.hpp"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace storepulse {
namespace {

QVariant bindText(const QString& value)
{
    return value.isEmpty() ? QStringLiteral("") : value;
}

} // namespace

FailureHitRepository::FailureHitRepository(SqliteDatabase& db)
    : m_db(db)
{
}

QString FailureHitRepository::lastError() const
{
    return m_lastError;
}

bool FailureHitRepository::upsertUnlocked(const FailureHitRow& row)
{
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "INSERT INTO failure_hits ("
        "  store_id, date_utc, failure_name, failure_hash, failure_stack_hash,"
        "  os_version, os_architecture, package_name, package_version, event_type, event_count,"
        "  cab_id, stack_trace, first_seen_at, last_seen_at, ingested_at, restated_at"
        ") VALUES ("
        "  ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,"
        "  strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), NULL"
        ") "
        "ON CONFLICT ("
        "  store_id, date_utc, failure_hash, failure_stack_hash,"
        "  os_version, os_architecture, package_name, package_version, event_type, cab_id"
        ") DO UPDATE SET "
        "  failure_name = excluded.failure_name,"
        "  event_count = excluded.event_count,"
        "  stack_trace = COALESCE(excluded.stack_trace, failure_hits.stack_trace),"
        "  first_seen_at = COALESCE(failure_hits.first_seen_at, excluded.first_seen_at),"
        "  last_seen_at = excluded.last_seen_at,"
        "  restated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')"));

    q.addBindValue(bindText(row.storeId));
    q.addBindValue(bindText(row.dateUtc));
    q.addBindValue(bindText(row.failureName));
    q.addBindValue(bindText(row.failureHash));
    q.addBindValue(bindText(row.failureStackHash));
    q.addBindValue(bindText(row.osVersion));
    q.addBindValue(bindText(row.osArchitecture));
    q.addBindValue(bindText(row.packageName));
    q.addBindValue(bindText(row.packageVersion));
    q.addBindValue(bindText(row.eventType));
    q.addBindValue(row.eventCount);
    q.addBindValue(bindText(row.cabId));
    q.addBindValue(row.stackTrace.isEmpty() ? QVariant() : QVariant(row.stackTrace));
    q.addBindValue(row.firstSeenAt.isEmpty() ? QVariant() : QVariant(row.firstSeenAt));
    q.addBindValue(row.lastSeenAt.isEmpty() ? QVariant() : QVariant(row.lastSeenAt));

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool FailureHitRepository::upsert(const FailureHitRow& row)
{
    m_lastError.clear();
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }
    if (!upsertUnlocked(row)) {
        (void)m_db.rollback();
        return false;
    }
    if (!m_db.commit()) {
        m_lastError = m_db.lastError();
        return false;
    }
    return true;
}

bool FailureHitRepository::upsertBatch(const QVector<FailureHitRow>& rows)
{
    m_lastError.clear();
    if (rows.isEmpty()) {
        return true;
    }
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }
    for (const FailureHitRow& row : rows) {
        if (!upsertUnlocked(row)) {
            (void)m_db.rollback();
            return false;
        }
    }
    if (!m_db.commit()) {
        m_lastError = m_db.lastError();
        return false;
    }
    return true;
}

qint64 FailureHitRepository::sumEvents(const QString& storeId,
                                       const QString& packageVersion,
                                       const QDate& fromUtc,
                                       const QDate& toUtc) const
{
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(event_count), 0) FROM failure_hits "
        "WHERE store_id = ? AND package_version = ? "
        "  AND date_utc >= ? AND date_utc <= ?"));
    q.addBindValue(storeId);
    q.addBindValue(packageVersion);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec() || !q.next()) {
        m_lastError = q.lastError().text();
        return 0;
    }
    return q.value(0).toLongLong();
}

} // namespace storepulse
