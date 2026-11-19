#include "AcquisitionRepository.hpp"

#include "Logger.hpp"

#include <QSqlError>
#include <QSqlQuery>

namespace storepulse {
namespace {

QVariant bindText(const QString& value)
{
    return value.isEmpty() ? QStringLiteral("") : value;
}

} // namespace

AcquisitionRepository::AcquisitionRepository(SqliteDatabase& db)
    : m_db(db)
{
}

QString AcquisitionRepository::lastError() const
{
    return m_lastError;
}

bool AcquisitionRepository::upsertUnlocked(const AcquisitionRow& row)
{
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "INSERT INTO acquisitions ("
        "  store_id, date_utc, granularity, acquisition_type, market, device_type, os_version,"
        "  age_group, gender, store_client, channel, campaign_id, quantity,"
        "  purchase_price_usd, purchase_tax_usd, source,"
        "  ingested_at, restated_at"
        ") VALUES ("
        "  ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,"
        "  strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), NULL"
        ") "
        "ON CONFLICT ("
        "  store_id, date_utc, granularity, acquisition_type, market,"
        "  device_type, os_version, age_group, gender, store_client, channel, campaign_id, source"
        ") DO UPDATE SET "
        "  quantity = excluded.quantity,"
        "  purchase_price_usd = excluded.purchase_price_usd,"
        "  purchase_tax_usd = excluded.purchase_tax_usd,"
        "  restated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')"));

    q.addBindValue(bindText(row.storeId));
    q.addBindValue(bindText(row.dateUtc));
    q.addBindValue(bindText(row.granularity.isEmpty() ? QStringLiteral("daily") : row.granularity));
    q.addBindValue(bindText(row.acquisitionType));
    q.addBindValue(bindText(row.market));
    q.addBindValue(bindText(row.deviceType));
    q.addBindValue(bindText(row.osVersion));
    q.addBindValue(bindText(row.ageGroup));
    q.addBindValue(bindText(row.gender));
    q.addBindValue(bindText(row.storeClient));
    q.addBindValue(bindText(row.channel));
    q.addBindValue(bindText(row.campaignId));
    q.addBindValue(row.quantity);
    q.addBindValue(row.purchasePriceUsd);
    q.addBindValue(row.purchaseTaxUsd);
    q.addBindValue(bindText(row.source.isEmpty() ? QStringLiteral("analytics") : row.source));

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool AcquisitionRepository::upsert(const AcquisitionRow& row)
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

bool AcquisitionRepository::upsertBatch(const QVector<AcquisitionRow>& rows)
{
    m_lastError.clear();
    if (rows.isEmpty()) {
        return true;
    }
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }
    for (const AcquisitionRow& row : rows) {
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

bool AcquisitionRepository::clearUndimensionedWindow(const QString& storeId,
                                                     const QDate& fromUtc,
                                                     const QDate& toUtc)
{
    m_lastError.clear();
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "DELETE FROM acquisitions WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? "
        "AND market = '' AND device_type = '' AND os_version = '' AND acquisition_type = ''"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool AcquisitionRepository::clearWindow(const QString& storeId,
                                        const QDate& fromUtc,
                                        const QDate& toUtc)
{
    m_lastError.clear();
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "DELETE FROM acquisitions WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool AcquisitionRepository::upsertRestatementWindow(const QString& storeId,
                                                    const QDate& fromUtc,
                                                    const QDate& toUtc,
                                                    const QVector<AcquisitionRow>& rows)
{
    m_lastError.clear();
    if (!fromUtc.isValid() || !toUtc.isValid() || fromUtc > toUtc) {
        m_lastError = QStringLiteral("Invalid restatement window");
        return false;
    }

    Q_UNUSED(storeId);
    Q_UNUSED(fromUtc);
    Q_UNUSED(toUtc);

    Logger::info(QStringLiteral("Acquisition restatement upsert: %1 rows").arg(rows.size()));
    return upsertBatch(rows);
}

qint64 AcquisitionRepository::sumQuantity(const QString& storeId,
                                          const QDate& fromUtc,
                                          const QDate& toUtc) const
{
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(quantity), 0) FROM acquisitions "
        "WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec() || !q.next()) {
        m_lastError = q.lastError().text();
        return 0;
    }
    return q.value(0).toLongLong();
}

} // namespace storepulse
