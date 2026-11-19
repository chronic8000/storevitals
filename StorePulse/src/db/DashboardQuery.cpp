#include "DashboardQuery.hpp"

#include <QHash>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>

namespace storepulse {
namespace {

QString rankLabel(QString raw)
{
    const QString trimmed = raw.trimmed();
    if (trimmed.isEmpty()
        || trimmed.compare(QLatin1String("(unknown)"), Qt::CaseInsensitive) == 0
        || trimmed.compare(QLatin1String("unknown"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("Unknown");
    }
    return trimmed;
}

} // namespace

DashboardQuery::DashboardQuery(SqliteDatabase& db)
    : m_db(db)
{
}

QString DashboardQuery::lastError() const { return m_lastError; }

QVariantList DashboardQuery::listAppsWithStats(const QDate& fromUtc, const QDate& toUtc) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT a.store_id, a.display_name, "
        "  COALESCE((SELECT SUM(quantity) FROM acquisitions x "
        "            WHERE x.store_id = a.store_id AND x.date_utc >= ? AND x.date_utc <= ?), 0), "
        "  COALESCE(("
        "    SELECT SUM(pageviews_count) FROM insights_pageviews p "
        "    WHERE p.store_id = a.store_id AND p.date_utc >= ? AND p.date_utc <= ?"
        "  ), ("
        "    SELECT SUM(click_count) FROM channel_conversions c "
        "    WHERE c.store_id = a.store_id AND c.date_utc >= ? AND c.date_utc <= ?"
        "  ), 0), "
        "  COALESCE((SELECT SUM(successful_install_count) FROM installs i "
        "            WHERE i.store_id = a.store_id AND i.date_utc >= ? AND i.date_utc <= ?), 0) "
        "FROM apps a WHERE a.is_active = 1 "
        "ORDER BY a.display_name COLLATE NOCASE"));
    const QString from = fromUtc.toString(Qt::ISODate);
    const QString to = toUtc.toString(Qt::ISODate);
    q.addBindValue(from);
    q.addBindValue(to);
    q.addBindValue(from);
    q.addBindValue(to);
    q.addBindValue(from);
    q.addBindValue(to);
    q.addBindValue(from);
    q.addBindValue(to);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("storeId"), q.value(0));
        row.insert(QStringLiteral("displayName"), q.value(1));
        row.insert(QStringLiteral("acquisitions"), q.value(2).toLongLong());
        row.insert(QStringLiteral("pageViews"), q.value(3).toLongLong());
        row.insert(QStringLiteral("installs"), q.value(4).toLongLong());
        out.push_back(row);
    }
    return out;
}

QVariantMap DashboardQuery::funnel(const QString& storeId, const QDate& fromUtc, const QDate& toUtc) const
{
    QVariantMap map;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT metric_type, user_count FROM funnel_metrics "
        "WHERE store_id = ? AND window_start = ? AND window_end = ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        // Fall back to latest funnel snapshot for this app.
        QSqlQuery q2(m_db.database());
        q2.prepare(QStringLiteral(
            "SELECT metric_type, user_count FROM funnel_metrics "
            "WHERE store_id = ? AND window_end = ("
            "  SELECT MAX(window_end) FROM funnel_metrics WHERE store_id = ?)"));
        q2.addBindValue(storeId);
        q2.addBindValue(storeId);
        if (!q2.exec()) {
            m_lastError = q2.lastError().text();
            return map;
        }
        while (q2.next()) {
            map.insert(q2.value(0).toString(), q2.value(1).toLongLong());
        }
        return map;
    }
    while (q.next()) {
        map.insert(q.value(0).toString(), q.value(1).toLongLong());
    }
    return map;
}

QVariantMap DashboardQuery::kpis(const QString& storeId, const QDate& fromUtc, const QDate& toUtc) const
{
    QVariantMap map = funnel(storeId, fromUtc, toUtc);
    const QString from = fromUtc.toString(Qt::ISODate);
    const QString to = toUtc.toString(Qt::ISODate);
    const QString today = QDate::currentDate().toString(Qt::ISODate);

    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(quantity),0), "
        "COALESCE(SUM(CASE WHEN date_utc = ? THEN quantity ELSE 0 END),0) "
        "FROM acquisitions WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
    q.addBindValue(today);
    q.addBindValue(storeId);
    q.addBindValue(from);
    q.addBindValue(to);
    if (q.exec() && q.next()) {
        map.insert(QStringLiteral("totalAcquisitions"), q.value(0).toLongLong());
        map.insert(QStringLiteral("todayAcquisitions"), q.value(1).toLongLong());
    }

    QSqlQuery i(m_db.database());
    i.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(successful_install_count),0) FROM installs "
        "WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
    i.addBindValue(storeId);
    i.addBindValue(from);
    i.addBindValue(to);
    if (i.exec() && i.next()) {
        map.insert(QStringLiteral("totalInstalls"), i.value(0).toLongLong());
    }

    QSqlQuery u(m_db.database());
    u.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(daily_active_users),0), COALESCE(SUM(engagement_minutes),0), "
        "COALESCE(SUM(daily_session_count),0), COALESCE(SUM(daily_active_devices),0), "
        "COALESCE(SUM(daily_new_users),0) "
        "FROM usage_daily WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
    u.addBindValue(storeId);
    u.addBindValue(from);
    u.addBindValue(to);
    if (u.exec() && u.next()) {
        map.insert(QStringLiteral("activeUsers"), u.value(0).toLongLong());
        map.insert(QStringLiteral("engagementMinutes"), u.value(1).toDouble());
        map.insert(QStringLiteral("sessions"), u.value(2).toLongLong());
        map.insert(QStringLiteral("activeDevices"), u.value(3).toLongLong());
        map.insert(QStringLiteral("newUsers"), u.value(4).toLongLong());
    }

    QSqlQuery ch(m_db.database());
    ch.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(conversion_count),0) FROM channel_conversions "
        "WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? AND date_utc != 'total'"));
    ch.addBindValue(storeId);
    ch.addBindValue(from);
    ch.addBindValue(to);
    if (ch.exec() && ch.next()) {
        map.insert(QStringLiteral("channelConversions"), ch.value(0).toLongLong());
    }

    // Gross sales (USD) — Partner Center Acquisitions "Gross sales", not net payouts.
    double appSales = 0;
    double iapSales = 0;
    double taxUsd = 0;
    QSqlQuery rev(m_db.database());
    rev.prepare(QStringLiteral(
        "SELECT source, COALESCE(SUM(purchase_price_usd),0), COALESCE(SUM(purchase_tax_usd),0) "
        "FROM revenue_daily "
        "WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? "
        "GROUP BY source"));
    rev.addBindValue(storeId);
    rev.addBindValue(from);
    rev.addBindValue(to);
    if (rev.exec()) {
        while (rev.next()) {
            const QString source = rev.value(0).toString();
            const double price = rev.value(1).toDouble();
            const double tax = rev.value(2).toDouble();
            taxUsd += tax;
            if (source == QLatin1String("iap")) {
                iapSales += price;
            } else {
                appSales += price;
            }
        }
    }
    // Fallback: money columns on dimensional acquisitions if revenue_daily not synced yet.
    if (appSales <= 0 && iapSales <= 0) {
        QSqlQuery acqRev(m_db.database());
        acqRev.prepare(QStringLiteral(
            "SELECT COALESCE(SUM(purchase_price_usd),0), COALESCE(SUM(purchase_tax_usd),0) "
            "FROM acquisitions WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
        acqRev.addBindValue(storeId);
        acqRev.addBindValue(from);
        acqRev.addBindValue(to);
        if (acqRev.exec() && acqRev.next()) {
            appSales = acqRev.value(0).toDouble();
            taxUsd = acqRev.value(1).toDouble();
        }
    }
    map.insert(QStringLiteral("appSalesUsd"), appSales);
    map.insert(QStringLiteral("iapSalesUsd"), iapSales);
    map.insert(QStringLiteral("taxUsd"), taxUsd);
    map.insert(QStringLiteral("grossSalesUsd"), appSales + iapSales);

    // Prefer all known ratings for the app (ratings are sparse / often older
    // than the selected analytics window).
    QSqlQuery r(m_db.database());
    r.prepare(QStringLiteral(
        "SELECT "
        "  COALESCE("
        "    (SUM(one_star)+2*SUM(two_star)+3*SUM(three_star)+4*SUM(four_star)+5*SUM(five_star))"
        "    / NULLIF(SUM(one_star+two_star+three_star+four_star+five_star),0), "
        "    COALESCE(SUM(rating_count*average_rating)/NULLIF(SUM(rating_count),0),0)"
        "  ), "
        "  COALESCE(SUM(one_star+two_star+three_star+four_star+five_star), "
        "           SUM(rating_count), 0) "
        "FROM ratings_daily WHERE store_id = ?"));
    r.addBindValue(storeId);
    if (r.exec() && r.next()) {
        map.insert(QStringLiteral("averageRating"), r.value(0).toDouble());
        map.insert(QStringLiteral("ratingCount"), r.value(1).toLongLong());
    }

    QSqlQuery f(m_db.database());
    f.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(event_count),0) FROM failure_hits "
        "WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
    f.addBindValue(storeId);
    f.addBindValue(from);
    f.addBindValue(to);
    if (f.exec() && f.next()) {
        map.insert(QStringLiteral("failureEvents"), f.value(0).toLongLong());
    }

    map.insert(QStringLiteral("funnelPageViews"), map.value(QStringLiteral("PageView")).toLongLong());
    map.insert(QStringLiteral("funnelAcquisition"), map.value(QStringLiteral("Acquisition")).toLongLong());
    map.insert(QStringLiteral("funnelInstall"), map.value(QStringLiteral("Install")).toLongLong());
    map.insert(QStringLiteral("funnelUsage"), map.value(QStringLiteral("Usage")).toLongLong());

    // Prefer Insights AcqV2 page views (Partner Center overview card). Fall back to
    // classic channel clickCount when Insights data has not been synced yet.
    qint64 pageViews = 0;
    QSqlQuery ipv(m_db.database());
    ipv.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(pageviews_count),0) FROM insights_pageviews "
        "WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
    ipv.addBindValue(storeId);
    ipv.addBindValue(from);
    ipv.addBindValue(to);
    if (ipv.exec() && ipv.next()) {
        pageViews = ipv.value(0).toLongLong();
    }
    qint64 listingClicks = 0;
    QSqlQuery pv(m_db.database());
    pv.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(click_count),0) FROM channel_conversions "
        "WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? AND date_utc != 'total'"));
    pv.addBindValue(storeId);
    pv.addBindValue(from);
    pv.addBindValue(to);
    if (pv.exec() && pv.next()) {
        listingClicks = pv.value(0).toLongLong();
    }
    if (pageViews <= 0) {
        pageViews = listingClicks;
    }
    map.insert(QStringLiteral("pageViews"), pageViews);
    map.insert(QStringLiteral("listingClicks"), listingClicks);
    map.insert(QStringLiteral("insightsPageViews"), pageViews);
    const qint64 installsForConv = map.value(QStringLiteral("totalInstalls")).toLongLong();
    map.insert(QStringLiteral("conversionRate"),
               pageViews > 0 ? (100.0 * double(installsForConv) / double(pageViews)) : 0.0);
    return map;
}

namespace {

QString dayKey(const QString& raw)
{
    // Normalize "2026-11-19", "2026-11-19T00:00:00Z", etc. to YYYY-MM-DD.
    if (raw.size() >= 10 && raw[4] == QLatin1Char('-') && raw[7] == QLatin1Char('-')) {
        return raw.left(10);
    }
    const QDate d = QDate::fromString(raw.left(10), Qt::ISODate);
    return d.isValid() ? d.toString(Qt::ISODate) : raw.left(10);
}

QVariantList zeroFilledDaily(const QHash<QString, qint64>& byDay,
                             const QDate& fromUtc,
                             const QDate& toUtc)
{
    QVariantList out;
    if (!fromUtc.isValid() || !toUtc.isValid() || fromUtc > toUtc) {
        return out;
    }
    out.reserve(fromUtc.daysTo(toUtc) + 1);
    for (QDate d = fromUtc; d <= toUtc; d = d.addDays(1)) {
        const QString key = d.toString(Qt::ISODate);
        QVariantMap row;
        row.insert(QStringLiteral("date"), key);
        row.insert(QStringLiteral("quantity"), byDay.value(key, 0));
        out.push_back(row);
    }
    return out;
}

} // namespace

QVariantList DashboardQuery::dailyAcquisitions(const QString& storeId,
                                               const QDate& fromUtc,
                                               const QDate& toUtc) const
{
    QHash<QString, qint64> byDay;
    QSqlQuery q(m_db.database());
    // Compare on the date prefix so timestamped rows still fall in-range.
    q.prepare(QStringLiteral(
        "SELECT substr(date_utc, 1, 10), SUM(quantity) FROM acquisitions "
        "WHERE store_id = ? AND substr(date_utc, 1, 10) >= ? AND substr(date_utc, 1, 10) <= ? "
        "GROUP BY substr(date_utc, 1, 10) ORDER BY 1"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return zeroFilledDaily({}, fromUtc, toUtc);
    }
    while (q.next()) {
        const QString key = dayKey(q.value(0).toString());
        byDay[key] = byDay.value(key, 0) + q.value(1).toLongLong();
    }
    return zeroFilledDaily(byDay, fromUtc, toUtc);
}

QVariantList DashboardQuery::dailyInstalls(const QString& storeId,
                                           const QDate& fromUtc,
                                           const QDate& toUtc) const
{
    QHash<QString, qint64> byDay;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT substr(date_utc, 1, 10), SUM(successful_install_count) FROM installs "
        "WHERE store_id = ? AND substr(date_utc, 1, 10) >= ? AND substr(date_utc, 1, 10) <= ? "
        "GROUP BY substr(date_utc, 1, 10) ORDER BY 1"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return zeroFilledDaily({}, fromUtc, toUtc);
    }
    while (q.next()) {
        const QString key = dayKey(q.value(0).toString());
        byDay[key] = byDay.value(key, 0) + q.value(1).toLongLong();
    }
    return zeroFilledDaily(byDay, fromUtc, toUtc);
}

QVariantList DashboardQuery::breakdown(const QString& storeId,
                                       const QString& table,
                                       const QString& column,
                                       const QString& valueColumn,
                                       const QDate& fromUtc,
                                       const QDate& toUtc,
                                       int limit) const
{
    QVariantList out;
    static const QSet<QString> tables = {
        QStringLiteral("acquisitions"), QStringLiteral("installs"), QStringLiteral("usage_daily")};
    static const QSet<QString> cols = {
        QStringLiteral("market"), QStringLiteral("device_type"), QStringLiteral("os_version"),
        QStringLiteral("acquisition_type"), QStringLiteral("package_version"),
        QStringLiteral("store_client"), QStringLiteral("age_group"), QStringLiteral("gender"),
        QStringLiteral("channel"), QStringLiteral("campaign_id")};
    static const QSet<QString> vals = {
        QStringLiteral("quantity"), QStringLiteral("successful_install_count"),
        QStringLiteral("daily_active_users"), QStringLiteral("daily_session_count"),
        QStringLiteral("daily_new_users"), QStringLiteral("daily_active_devices")};
    if (!tables.contains(table) || !cols.contains(column) || !vals.contains(valueColumn)) {
        m_lastError = QStringLiteral("Invalid breakdown");
        return out;
    }

    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
                  "SELECT CASE WHEN %1 = '' OR %1 IS NULL THEN '(unknown)' ELSE %1 END AS label, "
                  "SUM(%2) AS qty FROM %3 "
                  "WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? "
                  "GROUP BY label HAVING qty > 0 ORDER BY qty DESC LIMIT ?")
                  .arg(column, valueColumn, table));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), rankLabel(q.value(0).toString()));
        row.insert(QStringLiteral("value"), q.value(1).toLongLong());
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::packageVersions(const QString& storeId,
                                             const QDate& fromUtc,
                                             const QDate& toUtc) const
{
    return breakdown(storeId,
                     QStringLiteral("installs"),
                     QStringLiteral("package_version"),
                     QStringLiteral("successful_install_count"),
                     fromUtc,
                     toUtc,
                     8);
}

QVariantList DashboardQuery::versionHealth(const QString& storeId,
                                           const QDate& fromUtc,
                                           const QDate& toUtc,
                                           int limit) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "WITH versions AS ("
        "  SELECT package_version AS v FROM installs "
        "  WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? AND package_version != '' "
        "  UNION "
        "  SELECT package_version FROM usage_daily "
        "  WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? AND package_version != '' "
        "  UNION "
        "  SELECT package_version FROM failure_hits "
        "  WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? AND package_version != '' "
        ") "
        "SELECT v, "
        "  COALESCE((SELECT SUM(successful_install_count) FROM installs i "
        "            WHERE i.store_id = ? AND i.date_utc >= ? AND i.date_utc <= ? "
        "              AND i.package_version = versions.v), 0) AS installs, "
        "  COALESCE((SELECT SUM(daily_active_users) FROM usage_daily u "
        "            WHERE u.store_id = ? AND u.date_utc >= ? AND u.date_utc <= ? "
        "              AND u.package_version = versions.v), 0) AS active_users, "
        "  COALESCE((SELECT SUM(event_count) FROM failure_hits f "
        "            WHERE f.store_id = ? AND f.date_utc >= ? AND f.date_utc <= ? "
        "              AND f.package_version = versions.v), 0) AS crashes "
        "FROM versions "
        "ORDER BY installs DESC, crashes DESC, v DESC "
        "LIMIT ?"));
    const QString from = fromUtc.toString(Qt::ISODate);
    const QString to = toUtc.toString(Qt::ISODate);
    // versions CTE: 3× (store, from, to)
    for (int i = 0; i < 3; ++i) {
        q.addBindValue(storeId);
        q.addBindValue(from);
        q.addBindValue(to);
    }
    // three correlated subqueries
    for (int i = 0; i < 3; ++i) {
        q.addBindValue(storeId);
        q.addBindValue(from);
        q.addBindValue(to);
    }
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        const QString version = q.value(0).toString();
        const qint64 installs = q.value(1).toLongLong();
        const qint64 active = q.value(2).toLongLong();
        const qint64 crashes = q.value(3).toLongLong();
        QString flag;
        if (installs > 0 && installs <= 5 && active <= 0) {
            flag = QStringLiteral("thin release — few installs, no usage");
        } else if (installs > 0 && installs <= 10 && crashes > installs) {
            flag = QStringLiteral("more crashes than installs");
        } else if (installs > 0 && active <= 0 && crashes > 0) {
            flag = QStringLiteral("crashes with no recorded active users");
        }
        QVariantMap row;
        row.insert(QStringLiteral("name"), version);
        row.insert(QStringLiteral("installs"), installs);
        row.insert(QStringLiteral("activeUsers"), active);
        row.insert(QStringLiteral("crashes"), crashes);
        row.insert(QStringLiteral("flag"), flag);
        row.insert(QStringLiteral("value"), installs); // RankList-compatible
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::channelBreakdown(const QString& storeId,
                                              const QDate& fromUtc,
                                              const QDate& toUtc,
                                              int limit) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT CASE WHEN channel_type = '' OR channel_type IS NULL THEN '(unknown)' "
        "ELSE channel_type END AS label, "
        "SUM(click_count) AS clicks, SUM(conversion_count) AS conversions "
        "FROM channel_conversions "
        "WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? AND date_utc != 'total' "
        "GROUP BY label HAVING clicks > 0 OR conversions > 0 "
        "ORDER BY clicks DESC LIMIT ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), rankLabel(q.value(0).toString()));
        row.insert(QStringLiteral("value"), q.value(1).toLongLong());
        row.insert(QStringLiteral("conversions"), q.value(2).toLongLong());
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::ratingStarBreakdown(const QString& storeId) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(one_star),0), COALESCE(SUM(two_star),0), COALESCE(SUM(three_star),0), "
        "COALESCE(SUM(four_star),0), COALESCE(SUM(five_star),0) "
        "FROM ratings_daily WHERE store_id = ?"));
    q.addBindValue(storeId);
    if (!q.exec() || !q.next()) {
        m_lastError = q.lastError().text();
        return out;
    }
    for (int star = 1; star <= 5; ++star) {
        const qint64 count = q.value(star - 1).toLongLong();
        if (count <= 0) {
            continue;
        }
        QVariantMap row;
        row.insert(QStringLiteral("name"), QStringLiteral("%1★").arg(star));
        row.insert(QStringLiteral("value"), count);
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::revenueProducts(const QString& storeId,
                                             const QDate& fromUtc,
                                             const QDate& toUtc,
                                             int limit) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT CASE "
        "  WHEN product_name != '' THEN product_name "
        "  WHEN product_id != '' THEN product_id "
        "  WHEN source = 'iap' THEN 'Add-on' "
        "  ELSE 'App' END AS label, "
        "SUM(purchase_price_usd) AS usd "
        "FROM revenue_daily "
        "WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? "
        "GROUP BY label HAVING usd > 0 ORDER BY usd DESC LIMIT ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), q.value(0));
        // Keep cents as integer cents * 100 for fmtMoney via double in QML.
        row.insert(QStringLiteral("value"), q.value(1).toDouble());
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::referrerBreakdown(const QString& storeId,
                                               const QDate& fromUtc,
                                               const QDate& toUtc,
                                               int limit) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT CASE WHEN referrer_uri_domain = '' OR referrer_uri_domain IS NULL "
        "THEN '(unknown)' ELSE referrer_uri_domain END AS label, "
        "SUM(click_count) AS clicks "
        "FROM channel_conversions "
        "WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? AND date_utc != 'total' "
        "GROUP BY label HAVING clicks > 0 "
        "ORDER BY clicks DESC LIMIT ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), rankLabel(q.value(0).toString()));
        row.insert(QStringLiteral("value"), q.value(1).toLongLong());
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::campaignBreakdown(const QString& storeId,
                                               const QDate& fromUtc,
                                               const QDate& toUtc,
                                               int limit) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT CASE WHEN custom_campaign_id = '' OR custom_campaign_id IS NULL "
        "THEN '(unknown)' ELSE custom_campaign_id END AS label, "
        "SUM(click_count) AS clicks, SUM(conversion_count) AS conversions "
        "FROM channel_conversions "
        "WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? AND date_utc != 'total' "
        "GROUP BY label HAVING clicks > 0 OR conversions > 0 "
        "ORDER BY clicks DESC LIMIT ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), rankLabel(q.value(0).toString()));
        row.insert(QStringLiteral("value"), q.value(1).toLongLong());
        row.insert(QStringLiteral("conversions"), q.value(2).toLongLong());
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::architectureBreakdown(const QString& storeId,
                                                   const QDate& fromUtc,
                                                   const QDate& toUtc) const
{
    QVariantList out;
    // Same unit as the Top failures arch column: failure_hits event_count.
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT arch, SUM(event_count) AS qty FROM ("
        "  SELECT CASE "
        "    WHEN os_architecture != '' THEN lower(os_architecture) "
        "    WHEN lower(package_name) LIKE '%_arm64_%' THEN 'arm64' "
        "    WHEN lower(package_name) LIKE '%_x64_%' THEN 'x64' "
        "    WHEN lower(package_name) LIKE '%_x86_%' THEN 'x86' "
        "    ELSE '' END AS arch, event_count "
        "  FROM failure_hits "
        "  WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"
        ") WHERE arch != '' GROUP BY arch ORDER BY qty DESC"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), rankLabel(q.value(0).toString()));
        row.insert(QStringLiteral("value"), q.value(1).toLongLong());
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::recentReviews(const QString& storeId, int limit, bool lowRamOnly) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    QString sql = QStringLiteral(
        "SELECT date_utc, rating, title, review_text, market, package_version, reviewer_name, "
        "device_model, device_ram_mb, device_storage_gb, device_screen_resolution, "
        "is_revised, topic_tags, review_id, helpful_count, os_version, device_type, "
        "product_family, is_touch_enabled "
        "FROM reviews WHERE store_id = ? ");
    if (lowRamOnly) {
        sql += QStringLiteral("AND device_ram_mb > 0 AND device_ram_mb <= 8192 ");
    }
    sql += QStringLiteral("ORDER BY date_utc DESC LIMIT ?");
    q.prepare(sql);
    q.addBindValue(storeId);
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("date"), q.value(0));
        row.insert(QStringLiteral("rating"), q.value(1).toInt());
        row.insert(QStringLiteral("title"), q.value(2));
        row.insert(QStringLiteral("text"), q.value(3));
        row.insert(QStringLiteral("market"), q.value(4));
        row.insert(QStringLiteral("packageVersion"), q.value(5));
        row.insert(QStringLiteral("reviewer"), q.value(6));
        row.insert(QStringLiteral("deviceModel"), q.value(7));
        row.insert(QStringLiteral("deviceRamMb"), q.value(8).toLongLong());
        row.insert(QStringLiteral("deviceStorageGb"), q.value(9).toDouble());
        row.insert(QStringLiteral("screen"), q.value(10));
        row.insert(QStringLiteral("isRevised"), q.value(11).toInt() != 0);
        row.insert(QStringLiteral("topicTags"), q.value(12));
        row.insert(QStringLiteral("reviewId"), q.value(13));
        row.insert(QStringLiteral("helpfulCount"), q.value(14).toLongLong());
        row.insert(QStringLiteral("osVersion"), q.value(15));
        row.insert(QStringLiteral("deviceType"), q.value(16));
        row.insert(QStringLiteral("productFamily"), q.value(17));
        row.insert(QStringLiteral("isTouchEnabled"), q.value(18).toInt() != 0);
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::topFailures(const QString& storeId,
                                         const QDate& fromUtc,
                                         const QDate& toUtc,
                                         int limit) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    // Prefer real failure_name when Partner Center returns it. AQEMU currently
    // ships empty names/hashes — fall back to package + arch (+ event type).
    q.prepare(QStringLiteral(
        "SELECT "
        "  CASE "
        "    WHEN h.failure_name IS NOT NULL AND trim(h.failure_name) != '' THEN h.failure_name "
        "    ELSE trim( "
        "      CASE WHEN h.event_type != '' THEN h.event_type || ' · ' ELSE 'crash · ' END "
        "      || CASE "
        "           WHEN h.os_architecture != '' THEN lower(h.os_architecture) "
        "           WHEN lower(h.package_name) LIKE '%_arm64_%' THEN 'arm64' "
        "           WHEN lower(h.package_name) LIKE '%_x64_%' THEN 'x64' "
        "           WHEN lower(h.package_name) LIKE '%_x86_%' THEN 'x86' "
        "           ELSE 'unknown-arch' END "
        "      || CASE WHEN h.package_version != '' THEN ' · v' || h.package_version ELSE '' END "
        "    ) END AS label, "
        "  h.failure_hash, "
        "  CASE "
        "    WHEN h.os_architecture != '' THEN lower(h.os_architecture) "
        "    WHEN lower(h.package_name) LIKE '%_arm64_%' THEN 'arm64' "
        "    WHEN lower(h.package_name) LIKE '%_x64_%' THEN 'x64' "
        "    WHEN lower(h.package_name) LIKE '%_x86_%' THEN 'x86' "
        "    ELSE '' END AS arch, "
        "  SUM(h.event_count) AS qty, "
        "  COALESCE(("
        "    SELECT substr(d.stack_trace, 1, 240) FROM failure_details d "
        "    WHERE d.store_id = h.store_id "
        "      AND ( "
        "        (h.failure_hash != '' AND d.failure_hash = h.failure_hash) "
        "        OR (h.failure_hash = '' AND d.os_architecture = h.os_architecture "
        "            AND d.package_version = h.package_version) "
        "      ) AND length(d.stack_trace) > 0 LIMIT 1"
        "  ), COALESCE(MAX(h.stack_trace), '')) AS stack_preview "
        "FROM failure_hits h "
        "WHERE h.store_id = ? AND h.date_utc >= ? AND h.date_utc <= ? "
        "GROUP BY label, h.failure_hash, arch "
        "ORDER BY qty DESC LIMIT ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        QString name = q.value(0).toString();
        if (name.size() > 90) {
            name = name.left(87) + QStringLiteral("...");
        }
        row.insert(QStringLiteral("name"), name);
        row.insert(QStringLiteral("failureHash"), q.value(1));
        row.insert(QStringLiteral("architecture"), q.value(2));
        row.insert(QStringLiteral("value"), q.value(3).toLongLong());
        const QString stack = q.value(4).toString();
        row.insert(QStringLiteral("stackPreview"), stack);
        row.insert(QStringLiteral("hasStack"), !stack.isEmpty());
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::failureDeviceModels(const QString& storeId,
                                                 const QDate& fromUtc,
                                                 const QDate& toUtc,
                                                 int limit) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT CASE WHEN device_model = '' OR device_model IS NULL THEN '(unknown)' "
        "ELSE device_model END AS label, SUM(CASE WHEN event_count > 0 THEN event_count ELSE 1 END) AS qty "
        "FROM failure_details "
        "WHERE store_id = ? AND substr(date_utc, 1, 10) >= ? AND substr(date_utc, 1, 10) <= ? "
        "GROUP BY label ORDER BY qty DESC LIMIT ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), rankLabel(q.value(0).toString()));
        row.insert(QStringLiteral("value"), q.value(1).toLongLong());
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::failureCpuFamilies(const QString& storeId,
                                                const QDate& fromUtc,
                                                const QDate& toUtc,
                                                int limit) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    // Prefer manufacturer when family/name is blank or the literal "Unknown"
    // so Qualcomm/Snapdragon rows aren't collapsed into a blank "Unknown" label.
    q.prepare(QStringLiteral(
        "SELECT CASE "
        "  WHEN cpu_manufacturer != '' AND ( "
        "        lower(COALESCE(cpu_name,'')) IN ('', 'unknown') "
        "        AND lower(COALESCE(cpu_family,'')) IN ('', 'unknown') "
        "       ) THEN cpu_manufacturer "
        "  WHEN cpu_name != '' AND lower(cpu_name) != 'unknown' THEN cpu_name "
        "  WHEN cpu_family != '' AND lower(cpu_family) != 'unknown' THEN cpu_family "
        "  WHEN cpu_manufacturer != '' THEN cpu_manufacturer "
        "  ELSE '(unknown)' END AS label, "
        "SUM(CASE WHEN event_count > 0 THEN event_count ELSE 1 END) AS qty "
        "FROM failure_details "
        "WHERE store_id = ? AND substr(date_utc, 1, 10) >= ? AND substr(date_utc, 1, 10) <= ? "
        "GROUP BY label ORDER BY qty DESC LIMIT ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), rankLabel(q.value(0).toString()));
        row.insert(QStringLiteral("value"), q.value(1).toLongLong());
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::failureInsiderBreakdown(const QString& storeId,
                                                     const QDate& fromUtc,
                                                     const QDate& toUtc) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT CASE "
        "  WHEN lower(os_release) LIKE '%insider%' OR lower(flight_ring) LIKE '%insider%' "
        "    OR lower(os_release) IN ('external','canary','dev','beta') "
        "    OR lower(flight_ring) IN ('external','canary','dev','beta') THEN 'Insider / flight' "
        "  WHEN os_release != '' OR flight_ring != '' OR sandbox_id != '' THEN 'Retail' "
        "  ELSE 'Unknown' END AS label, "
        "SUM(CASE WHEN event_count > 0 THEN event_count ELSE 1 END) AS qty "
        "FROM failure_details "
        "WHERE store_id = ? AND substr(date_utc, 1, 10) >= ? AND substr(date_utc, 1, 10) <= ? "
        "GROUP BY label ORDER BY qty DESC"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), rankLabel(q.value(0).toString()));
        row.insert(QStringLiteral("value"), q.value(1).toLongLong());
        out.push_back(row);
    }
    return out;
}

QVariantList DashboardQuery::failureCabDownloads(const QString& storeId, int limit) const
{
    QVariantList out;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT failure_name, failure_hash, cab_id, cab_expiration, cab_downloadable, "
        "os_architecture, device_model, "
        "CASE WHEN length(stack_trace) > 0 THEN 1 ELSE 0 END AS has_stack "
        "FROM failure_details "
        "WHERE store_id = ? AND cab_id != '' AND cab_id NOT LIKE 'hit:%' "
        "AND cab_downloadable = 1 "
        "GROUP BY cab_id "
        "ORDER BY cab_expiration DESC LIMIT ?"));
    q.addBindValue(storeId);
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        QVariantMap row;
        QString name = q.value(0).toString();
        if (name.size() > 72) {
            name = name.left(69) + QStringLiteral("...");
        }
        row.insert(QStringLiteral("name"), name);
        row.insert(QStringLiteral("failureHash"), q.value(1));
        row.insert(QStringLiteral("cabId"), q.value(2));
        row.insert(QStringLiteral("cabExpiration"), q.value(3));
        row.insert(QStringLiteral("downloadable"), q.value(4).toInt() != 0);
        row.insert(QStringLiteral("architecture"), q.value(5));
        row.insert(QStringLiteral("deviceModel"), q.value(6));
        row.insert(QStringLiteral("hasStack"), q.value(7).toInt() != 0);
        out.push_back(row);
    }
    return out;
}

QString DashboardQuery::exportAcquisitionsCsv(const QString& storeId,
                                              const QDate& fromUtc,
                                              const QDate& toUtc) const
{
    QString csv = QStringLiteral(
        "date_utc,acquisition_type,market,device_type,os_version,store_client,quantity\n");
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT date_utc, acquisition_type, market, device_type, os_version, store_client, quantity "
        "FROM acquisitions WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? "
        "ORDER BY date_utc, market"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return {};
    }
    auto esc = [](QString t) {
        t.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        if (t.contains(QLatin1Char(',')) || t.contains(QLatin1Char('"'))) {
            return QStringLiteral("\"%1\"").arg(t);
        }
        return t;
    };
    while (q.next()) {
        QStringList cols;
        for (int i = 0; i < 7; ++i) {
            cols << esc(q.value(i).toString());
        }
        csv += cols.join(QLatin1Char(',')) + QLatin1Char('\n');
    }
    return csv;
}

QString DashboardQuery::exportInstallsCsv(const QString& storeId,
                                          const QDate& fromUtc,
                                          const QDate& toUtc) const
{
    QString csv = QStringLiteral(
        "date_utc,market,device_type,os_version,package_version,successful_install_count\n");
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT date_utc, market, device_type, os_version, package_version, successful_install_count "
        "FROM installs WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? ORDER BY date_utc"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return {};
    }
    auto esc = [](QString t) {
        t.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        if (t.contains(QLatin1Char(',')) || t.contains(QLatin1Char('"'))) {
            return QStringLiteral("\"%1\"").arg(t);
        }
        return t;
    };
    while (q.next()) {
        QStringList cols;
        for (int i = 0; i < 6; ++i) {
            cols << esc(q.value(i).toString());
        }
        csv += cols.join(QLatin1Char(',')) + QLatin1Char('\n');
    }
    return csv;
}

QString DashboardQuery::exportFailuresCsv(const QString& storeId,
                                          const QDate& fromUtc,
                                          const QDate& toUtc) const
{
    QString csv = QStringLiteral(
        "date_utc,failure_name,os_architecture,package_name,package_version,event_type,event_count\n");
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT date_utc, failure_name, os_architecture, package_name, package_version, event_type, event_count "
        "FROM failure_hits WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? ORDER BY date_utc"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return {};
    }
    auto esc = [](QString t) {
        t.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        if (t.contains(QLatin1Char(',')) || t.contains(QLatin1Char('"'))) {
            return QStringLiteral("\"%1\"").arg(t);
        }
        return t;
    };
    while (q.next()) {
        QStringList cols;
        for (int i = 0; i < 7; ++i) {
            cols << esc(q.value(i).toString());
        }
        csv += cols.join(QLatin1Char(',')) + QLatin1Char('\n');
    }
    return csv;
}

} // namespace storepulse
