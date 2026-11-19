#include "AppRepository.hpp"

#include <QSqlError>
#include <QSqlQuery>

namespace storepulse {
namespace {

QVariant bindText(const QString& value)
{
    return value.isEmpty() ? QStringLiteral("") : value;
}

} // namespace

AppRepository::AppRepository(SqliteDatabase& db)
    : m_db(db)
{
}

QString AppRepository::lastError() const
{
    return m_lastError;
}

bool AppRepository::upsert(const AppRow& row)
{
    m_lastError.clear();
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "INSERT INTO apps (store_id, display_name, package_family, is_active, first_seen_at, updated_at) "
        "VALUES (?, ?, ?, ?, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')) "
        "ON CONFLICT(store_id) DO UPDATE SET "
        "  display_name = CASE "
        "    WHEN excluded.display_name != '' AND excluded.display_name != excluded.store_id "
        "    THEN excluded.display_name ELSE apps.display_name END, "
        "  package_family = CASE WHEN excluded.package_family != '' "
        "    THEN excluded.package_family ELSE apps.package_family END, "
        "  is_active = excluded.is_active, "
        "  updated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')"));
    q.addBindValue(bindText(row.storeId));
    q.addBindValue(bindText(row.displayName.isEmpty() ? row.storeId : row.displayName));
    q.addBindValue(bindText(row.packageFamily));
    q.addBindValue(row.isActive ? 1 : 0);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool AppRepository::ensureApp(const QString& storeId, const QString& displayName)
{
    AppRow row;
    row.storeId = storeId.trimmed();
    row.displayName = displayName.trimmed().isEmpty() ? row.storeId : displayName.trimmed();
    row.isActive = true;
    return upsert(row);
}

QVector<AppRow> AppRepository::listActive() const
{
    QVector<AppRow> rows;
    QSqlQuery q(m_db.database());
    if (!q.exec(QStringLiteral(
            "SELECT store_id, display_name, package_family, is_active "
            "FROM apps WHERE is_active = 1 ORDER BY display_name COLLATE NOCASE"))) {
        m_lastError = q.lastError().text();
        return rows;
    }
    while (q.next()) {
        AppRow row;
        row.storeId = q.value(0).toString();
        row.displayName = q.value(1).toString();
        row.packageFamily = q.value(2).toString();
        row.isActive = q.value(3).toInt() != 0;
        rows.push_back(std::move(row));
    }
    return rows;
}

} // namespace storepulse
