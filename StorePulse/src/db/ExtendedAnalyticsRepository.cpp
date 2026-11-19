#include "ExtendedAnalyticsRepository.hpp"

#include <QSqlError>
#include <QSqlQuery>

namespace storepulse {
namespace {

QVariant bindText(const QString& value)
{
    return value.isEmpty() ? QStringLiteral("") : value;
}

} // namespace

ExtendedAnalyticsRepository::ExtendedAnalyticsRepository(SqliteDatabase& db)
    : m_db(db)
{
}

QString ExtendedAnalyticsRepository::lastError() const
{
    return m_lastError;
}

bool ExtendedAnalyticsRepository::replaceFunnel(const QString& storeId,
                                                const QDate& fromUtc,
                                                const QDate& toUtc,
                                                const QVector<FunnelMetricRow>& rows)
{
    m_lastError.clear();
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }

    QSqlQuery del(m_db.database());
    del.prepare(QStringLiteral(
        "DELETE FROM funnel_metrics WHERE store_id = ? AND window_start = ? AND window_end = ?"));
    del.addBindValue(storeId);
    del.addBindValue(fromUtc.toString(Qt::ISODate));
    del.addBindValue(toUtc.toString(Qt::ISODate));
    if (!del.exec()) {
        m_lastError = del.lastError().text();
        (void)m_db.rollback();
        return false;
    }

    for (const FunnelMetricRow& row : rows) {
        QSqlQuery q(m_db.database());
        q.prepare(QStringLiteral(
            "INSERT INTO funnel_metrics "
            "(store_id, window_start, window_end, metric_type, user_count, market, device_type) "
            "VALUES (?, ?, ?, ?, ?, ?, ?)"));
        q.addBindValue(bindText(storeId));
        q.addBindValue(bindText(fromUtc.toString(Qt::ISODate)));
        q.addBindValue(bindText(toUtc.toString(Qt::ISODate)));
        q.addBindValue(bindText(row.metricType));
        q.addBindValue(row.userCount);
        q.addBindValue(bindText(row.market));
        q.addBindValue(bindText(row.deviceType));
        if (!q.exec()) {
            m_lastError = q.lastError().text();
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

bool ExtendedAnalyticsRepository::clearInstallsWindow(const QString& storeId,
                                                      const QDate& fromUtc,
                                                      const QDate& toUtc)
{
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "DELETE FROM installs WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool ExtendedAnalyticsRepository::upsertInstalls(const QVector<InstallRow>& rows)
{
    m_lastError.clear();
    if (rows.isEmpty()) {
        return true;
    }
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }
    for (const InstallRow& row : rows) {
        QSqlQuery q(m_db.database());
        q.prepare(QStringLiteral(
            "INSERT INTO installs "
            "(store_id, date_utc, market, device_type, os_version, package_version, successful_install_count) "
            "VALUES (?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT (store_id, date_utc, market, device_type, os_version, package_version) DO UPDATE SET "
            "successful_install_count = excluded.successful_install_count, "
            "ingested_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')"));
        q.addBindValue(bindText(row.storeId));
        q.addBindValue(bindText(row.dateUtc));
        q.addBindValue(bindText(row.market));
        q.addBindValue(bindText(row.deviceType));
        q.addBindValue(bindText(row.osVersion));
        q.addBindValue(bindText(row.packageVersion));
        q.addBindValue(row.successfulInstallCount);
        if (!q.exec()) {
            m_lastError = q.lastError().text();
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

bool ExtendedAnalyticsRepository::replaceInstallsWindow(const QString& storeId,
                                                        const QDate& fromUtc,
                                                        const QDate& toUtc,
                                                        const QVector<InstallRow>& rows)
{
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }
    if (!clearInstallsWindow(storeId, fromUtc, toUtc)) {
        (void)m_db.rollback();
        return false;
    }
    if (!m_db.commit()) {
        m_lastError = m_db.lastError();
        return false;
    }
    return upsertInstalls(rows);
}

bool ExtendedAnalyticsRepository::clearUsageWindow(const QString& storeId,
                                                   const QDate& fromUtc,
                                                   const QDate& toUtc)
{
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "DELETE FROM usage_daily WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool ExtendedAnalyticsRepository::upsertUsage(const QVector<UsageDailyRow>& rows)
{
    m_lastError.clear();
    if (rows.isEmpty()) {
        return true;
    }
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }
    for (const UsageDailyRow& row : rows) {
        QSqlQuery q(m_db.database());
        q.prepare(QStringLiteral(
            "INSERT INTO usage_daily "
            "(store_id, date_utc, market, device_type, package_version, daily_session_count, "
            " engagement_minutes, daily_active_users, daily_active_devices, daily_new_users) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT (store_id, date_utc, market, device_type, package_version) DO UPDATE SET "
            "daily_session_count = excluded.daily_session_count, "
            "engagement_minutes = excluded.engagement_minutes, "
            "daily_active_users = excluded.daily_active_users, "
            "daily_active_devices = excluded.daily_active_devices, "
            "daily_new_users = excluded.daily_new_users, "
            "ingested_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')"));
        q.addBindValue(bindText(row.storeId));
        q.addBindValue(bindText(row.dateUtc));
        q.addBindValue(bindText(row.market));
        q.addBindValue(bindText(row.deviceType));
        q.addBindValue(bindText(row.packageVersion));
        q.addBindValue(row.dailySessionCount);
        q.addBindValue(row.engagementMinutes);
        q.addBindValue(row.dailyActiveUsers);
        q.addBindValue(row.dailyActiveDevices);
        q.addBindValue(row.dailyNewUsers);
        if (!q.exec()) {
            m_lastError = q.lastError().text();
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

bool ExtendedAnalyticsRepository::clearRatingsWindow(const QString& storeId,
                                                     const QDate& fromUtc,
                                                     const QDate& toUtc)
{
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "DELETE FROM ratings_daily WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool ExtendedAnalyticsRepository::upsertRatings(const QVector<RatingRow>& rows)
{
    m_lastError.clear();
    if (rows.isEmpty()) {
        return true;
    }
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }
    for (const RatingRow& row : rows) {
        QSqlQuery q(m_db.database());
        q.prepare(QStringLiteral(
            "INSERT INTO ratings_daily "
            "(store_id, date_utc, market, device_type, os_version, average_rating, rating_count, "
            " one_star, two_star, three_star, four_star, five_star) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT (store_id, date_utc, market, device_type, os_version) DO UPDATE SET "
            "average_rating = excluded.average_rating, rating_count = excluded.rating_count, "
            "one_star = excluded.one_star, two_star = excluded.two_star, "
            "three_star = excluded.three_star, four_star = excluded.four_star, "
            "five_star = excluded.five_star, "
            "ingested_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')"));
        q.addBindValue(bindText(row.storeId));
        q.addBindValue(bindText(row.dateUtc));
        q.addBindValue(bindText(row.market));
        q.addBindValue(bindText(row.deviceType));
        q.addBindValue(bindText(row.osVersion));
        q.addBindValue(row.averageRating);
        q.addBindValue(row.ratingCount);
        q.addBindValue(row.oneStar);
        q.addBindValue(row.twoStar);
        q.addBindValue(row.threeStar);
        q.addBindValue(row.fourStar);
        q.addBindValue(row.fiveStar);
        if (!q.exec()) {
            m_lastError = q.lastError().text();
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

bool ExtendedAnalyticsRepository::clearChannelWindow(const QString& storeId,
                                                     const QDate& fromUtc,
                                                     const QDate& toUtc)
{
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "DELETE FROM channel_conversions WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool ExtendedAnalyticsRepository::upsertChannelConversions(const QVector<ChannelConversionRow>& rows)
{
    m_lastError.clear();
    if (rows.isEmpty()) {
        return true;
    }
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }
    for (const ChannelConversionRow& row : rows) {
        QSqlQuery q(m_db.database());
        q.prepare(QStringLiteral(
            "INSERT INTO channel_conversions "
            "(store_id, date_utc, channel_type, market, device_type, "
            " referrer_uri_domain, custom_campaign_id, click_count, conversion_count) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT (store_id, date_utc, channel_type, market, device_type, "
            " referrer_uri_domain, custom_campaign_id) DO UPDATE SET "
            "click_count = excluded.click_count, conversion_count = excluded.conversion_count, "
            "ingested_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')"));
        q.addBindValue(bindText(row.storeId));
        q.addBindValue(bindText(row.dateUtc));
        q.addBindValue(bindText(row.channelType));
        q.addBindValue(bindText(row.market));
        q.addBindValue(bindText(row.deviceType));
        q.addBindValue(bindText(row.referrerUriDomain));
        q.addBindValue(bindText(row.customCampaignId));
        q.addBindValue(row.clickCount);
        q.addBindValue(row.conversionCount);
        if (!q.exec()) {
            m_lastError = q.lastError().text();
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

bool ExtendedAnalyticsRepository::clearInsightsPageviewsWindow(const QString& storeId,
                                                               const QDate& fromUtc,
                                                               const QDate& toUtc)
{
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "DELETE FROM insights_pageviews WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool ExtendedAnalyticsRepository::upsertInsightsPageviews(const QVector<InsightsPageviewRow>& rows)
{
    m_lastError.clear();
    if (rows.isEmpty()) {
        return true;
    }
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }
    for (const InsightsPageviewRow& row : rows) {
        QSqlQuery q(m_db.database());
        q.prepare(QStringLiteral(
            "INSERT INTO insights_pageviews (store_id, date_utc, pageviews_count) "
            "VALUES (?, ?, ?) "
            "ON CONFLICT (store_id, date_utc) DO UPDATE SET "
            "pageviews_count = excluded.pageviews_count, "
            "ingested_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')"));
        q.addBindValue(bindText(row.storeId));
        q.addBindValue(bindText(row.dateUtc));
        q.addBindValue(row.pageviewsCount);
        if (!q.exec()) {
            m_lastError = q.lastError().text();
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

bool ExtendedAnalyticsRepository::clearRevenueWindow(const QString& storeId,
                                                     const QDate& fromUtc,
                                                     const QDate& toUtc,
                                                     const QString& source)
{
    m_lastError.clear();
    QSqlQuery q(m_db.database());
    if (source.isEmpty()) {
        q.prepare(QStringLiteral(
            "DELETE FROM revenue_daily WHERE store_id = ? AND date_utc >= ? AND date_utc <= ?"));
        q.addBindValue(storeId);
        q.addBindValue(fromUtc.toString(Qt::ISODate));
        q.addBindValue(toUtc.toString(Qt::ISODate));
    } else {
        q.prepare(QStringLiteral(
            "DELETE FROM revenue_daily WHERE store_id = ? AND date_utc >= ? AND date_utc <= ? "
            "AND source = ?"));
        q.addBindValue(storeId);
        q.addBindValue(fromUtc.toString(Qt::ISODate));
        q.addBindValue(toUtc.toString(Qt::ISODate));
        q.addBindValue(source);
    }
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool ExtendedAnalyticsRepository::upsertRevenue(const QVector<RevenueRow>& rows)
{
    m_lastError.clear();
    if (rows.isEmpty()) {
        return true;
    }
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }
    for (const RevenueRow& row : rows) {
        QSqlQuery q(m_db.database());
        q.prepare(QStringLiteral(
            "INSERT INTO revenue_daily ("
            "  store_id, date_utc, source, product_id, product_name, acquisition_type, market,"
            "  quantity, purchase_price_usd, purchase_tax_usd"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT (store_id, date_utc, source, product_id, acquisition_type, market) "
            "DO UPDATE SET "
            "product_name = excluded.product_name, "
            "quantity = excluded.quantity, "
            "purchase_price_usd = excluded.purchase_price_usd, "
            "purchase_tax_usd = excluded.purchase_tax_usd, "
            "ingested_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')"));
        q.addBindValue(bindText(row.storeId));
        q.addBindValue(bindText(row.dateUtc));
        q.addBindValue(bindText(row.source.isEmpty() ? QStringLiteral("app") : row.source));
        q.addBindValue(bindText(row.productId));
        q.addBindValue(bindText(row.productName));
        q.addBindValue(bindText(row.acquisitionType));
        q.addBindValue(bindText(row.market));
        q.addBindValue(row.quantity);
        q.addBindValue(row.purchasePriceUsd);
        q.addBindValue(row.purchaseTaxUsd);
        if (!q.exec()) {
            m_lastError = q.lastError().text();
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

bool ExtendedAnalyticsRepository::upsertReviews(const QVector<ReviewRow>& rows)
{
    m_lastError.clear();
    if (rows.isEmpty()) {
        return true;
    }
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }
    for (const ReviewRow& row : rows) {
        QSqlQuery q(m_db.database());
        q.prepare(QStringLiteral(
            "INSERT INTO reviews "
            "(store_id, review_id, date_utc, rating, title, review_text, market, os_version, "
            " device_type, package_version, reviewer_name, helpful_count, "
            " device_model, device_ram_mb, device_storage_gb, device_screen_resolution, "
            " product_family, is_revised, is_touch_enabled, topic_tags) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT (store_id, review_id) DO UPDATE SET "
            "date_utc = excluded.date_utc, rating = excluded.rating, title = excluded.title, "
            "review_text = excluded.review_text, market = excluded.market, "
            "os_version = excluded.os_version, device_type = excluded.device_type, "
            "package_version = excluded.package_version, reviewer_name = excluded.reviewer_name, "
            "helpful_count = excluded.helpful_count, "
            "device_model = excluded.device_model, device_ram_mb = excluded.device_ram_mb, "
            "device_storage_gb = excluded.device_storage_gb, "
            "device_screen_resolution = excluded.device_screen_resolution, "
            "product_family = excluded.product_family, is_revised = excluded.is_revised, "
            "is_touch_enabled = excluded.is_touch_enabled, topic_tags = excluded.topic_tags, "
            "ingested_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')"));
        q.addBindValue(bindText(row.storeId));
        q.addBindValue(bindText(row.reviewId));
        q.addBindValue(bindText(row.dateUtc));
        q.addBindValue(row.rating);
        q.addBindValue(bindText(row.title));
        q.addBindValue(bindText(row.reviewText));
        q.addBindValue(bindText(row.market));
        q.addBindValue(bindText(row.osVersion));
        q.addBindValue(bindText(row.deviceType));
        q.addBindValue(bindText(row.packageVersion));
        q.addBindValue(bindText(row.reviewerName));
        q.addBindValue(row.helpfulCount);
        q.addBindValue(bindText(row.deviceModel));
        q.addBindValue(row.deviceRamMb);
        q.addBindValue(row.deviceStorageGb);
        q.addBindValue(bindText(row.deviceScreenResolution));
        q.addBindValue(bindText(row.productFamily));
        q.addBindValue(row.isRevised ? 1 : 0);
        q.addBindValue(row.isTouchEnabled ? 1 : 0);
        q.addBindValue(bindText(row.topicTags));
        if (!q.exec()) {
            m_lastError = q.lastError().text();
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

bool ExtendedAnalyticsRepository::clearFailureDetailsWindow(const QString& storeId,
                                                            const QDate& fromUtc,
                                                            const QDate& toUtc)
{
    m_lastError.clear();
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "DELETE FROM failure_details WHERE store_id = ? "
        "AND substr(date_utc, 1, 10) >= ? AND substr(date_utc, 1, 10) <= ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool ExtendedAnalyticsRepository::upsertFailureDetails(const QVector<FailureDetailRow>& rows)
{
    m_lastError.clear();
    if (rows.isEmpty()) {
        return true;
    }
    if (!m_db.beginTransaction()) {
        m_lastError = m_db.lastError();
        return false;
    }
    for (const FailureDetailRow& row : rows) {
        QSqlQuery q(m_db.database());
        q.prepare(QStringLiteral(
            "INSERT INTO failure_details ("
            "  store_id, failure_hash, failure_name, date_utc, cab_id, cab_expiration, "
            "  cab_downloadable, market, device_type, device_model, os_version, os_build, "
            "  os_release, os_architecture, package_version, cpu_manufacturer, cpu_family, "
            "  cpu_name, flight_ring, sandbox_id, event_count, stack_trace"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT (store_id, failure_hash, cab_id, date_utc, device_model, os_build, market) "
            "DO UPDATE SET "
            "failure_name = excluded.failure_name, "
            "cab_expiration = excluded.cab_expiration, "
            "cab_downloadable = excluded.cab_downloadable, "
            "device_type = excluded.device_type, "
            "os_version = excluded.os_version, "
            "os_release = excluded.os_release, "
            "os_architecture = excluded.os_architecture, "
            "package_version = excluded.package_version, "
            "cpu_manufacturer = excluded.cpu_manufacturer, "
            "cpu_family = excluded.cpu_family, "
            "cpu_name = excluded.cpu_name, "
            "flight_ring = excluded.flight_ring, "
            "sandbox_id = excluded.sandbox_id, "
            "event_count = excluded.event_count, "
            "stack_trace = CASE WHEN excluded.stack_trace != '' THEN excluded.stack_trace "
            "              ELSE failure_details.stack_trace END, "
            "ingested_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')"));
        q.addBindValue(bindText(row.storeId));
        q.addBindValue(bindText(row.failureHash));
        q.addBindValue(bindText(row.failureName));
        q.addBindValue(bindText(row.dateUtc));
        q.addBindValue(bindText(row.cabId));
        q.addBindValue(bindText(row.cabExpiration));
        q.addBindValue(row.cabDownloadable ? 1 : 0);
        q.addBindValue(bindText(row.market));
        q.addBindValue(bindText(row.deviceType));
        q.addBindValue(bindText(row.deviceModel));
        q.addBindValue(bindText(row.osVersion));
        q.addBindValue(bindText(row.osBuild));
        q.addBindValue(bindText(row.osRelease));
        q.addBindValue(bindText(row.osArchitecture));
        q.addBindValue(bindText(row.packageVersion));
        q.addBindValue(bindText(row.cpuManufacturer));
        q.addBindValue(bindText(row.cpuFamily));
        q.addBindValue(bindText(row.cpuName));
        q.addBindValue(bindText(row.flightRing));
        q.addBindValue(bindText(row.sandboxId));
        q.addBindValue(row.eventCount);
        q.addBindValue(bindText(row.stackTrace));
        if (!q.exec()) {
            m_lastError = q.lastError().text();
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

bool ExtendedAnalyticsRepository::updateFailureDetailStack(const QString& storeId,
                                                           const QString& cabId,
                                                           const QString& stackTrace)
{
    m_lastError.clear();
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "UPDATE failure_details SET stack_trace = ? "
        "WHERE store_id = ? AND cab_id = ?"));
    q.addBindValue(stackTrace);
    q.addBindValue(storeId);
    q.addBindValue(cabId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }

    // Also stamp matching failure_hits rows that share this cabId.
    QSqlQuery hits(m_db.database());
    hits.prepare(QStringLiteral(
        "UPDATE failure_hits SET stack_trace = ? "
        "WHERE store_id = ? AND cab_id = ? AND (stack_trace IS NULL OR stack_trace = '')"));
    hits.addBindValue(stackTrace);
    hits.addBindValue(storeId);
    hits.addBindValue(cabId);
    if (!hits.exec()) {
        m_lastError = hits.lastError().text();
        return false;
    }
    return true;
}

QStringList ExtendedAnalyticsRepository::topFailureHashes(const QString& storeId,
                                                          const QDate& fromUtc,
                                                          const QDate& toUtc,
                                                          int limit) const
{
    QStringList out;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT failure_hash, SUM(event_count) AS qty FROM failure_hits "
        "WHERE store_id = ? AND substr(date_utc, 1, 10) >= ? AND substr(date_utc, 1, 10) <= ? "
        "AND failure_hash != '' AND failure_hash != '_aggregate_' "
        "GROUP BY failure_hash ORDER BY qty DESC LIMIT ?"));
    q.addBindValue(storeId);
    q.addBindValue(fromUtc.toString(Qt::ISODate));
    q.addBindValue(toUtc.toString(Qt::ISODate));
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        out << q.value(0).toString();
    }
    return out;
}

QVector<QPair<QString, QString>> ExtendedAnalyticsRepository::stackCabTargets(const QString& storeId,
                                                                              int limit) const
{
    QVector<QPair<QString, QString>> out;
    QSqlQuery q(m_db.database());
    q.prepare(QStringLiteral(
        "SELECT failure_hash, cab_id FROM failure_details "
        "WHERE store_id = ? AND cab_id != '' AND cab_downloadable = 1 "
        "AND (stack_trace IS NULL OR stack_trace = '') "
        "GROUP BY failure_hash, cab_id LIMIT ?"));
    q.addBindValue(storeId);
    q.addBindValue(limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        out.push_back({q.value(0).toString(), q.value(1).toString()});
    }
    return out;
}

} // namespace storepulse
