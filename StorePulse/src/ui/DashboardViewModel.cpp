#include "DashboardViewModel.hpp"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>

#include <algorithm>
#include <utility>

namespace storepulse {

namespace {
constexpr int kMinWindowDays = 1;
constexpr int kMaxWindowDays = 365;
} // namespace

DashboardViewModel::DashboardViewModel(SqliteDatabase& db, QObject* parent)
    : QObject(parent)
    , m_db(db)
    , m_appsRepo(db)
    , m_query(db)
{
    const QDate today = QDate::currentDate();
    m_customFrom = today.addDays(-(m_windowDays - 1)).toString(Qt::ISODate);
    m_customTo = today.toString(Qt::ISODate);
    m_selectedStoreId = QStringLiteral("9P0HGKHQ9W19");
    refresh();
}

QString DashboardViewModel::normalizePath(const QString& filePath)
{
    if (filePath.startsWith(QLatin1String("file:"))) {
        return QUrl(filePath).toLocalFile();
    }
    return filePath;
}

QString DashboardViewModel::selectedDisplayName() const
{
    for (const QVariant& item : m_apps) {
        const QVariantMap row = item.toMap();
        if (row.value(QStringLiteral("storeId")).toString() == m_selectedStoreId) {
            return row.value(QStringLiteral("displayName")).toString();
        }
    }
    return m_selectedStoreId;
}

QPair<QDate, QDate> DashboardViewModel::activeWindow() const
{
    const QDate today = QDate::currentDate();
    if (m_customRange) {
        QDate from = QDate::fromString(m_customFrom, Qt::ISODate);
        QDate to = QDate::fromString(m_customTo, Qt::ISODate);
        if (!from.isValid()) {
            from = today.addDays(-(m_windowDays - 1));
        }
        if (!to.isValid()) {
            to = today;
        }
        if (from > to) {
            std::swap(from, to);
        }
        // Soft cap: keep queries within a year of local data / sync window.
        if (from.daysTo(to) > (kMaxWindowDays - 1)) {
            from = to.addDays(-(kMaxWindowDays - 1));
        }
        return {from, to};
    }
    return {today.addDays(-(m_windowDays - 1)), today};
}

QString DashboardViewModel::rangeLabel() const
{
    const auto [from, to] = activeWindow();
    if (m_customRange) {
        return QStringLiteral("%1 → %2").arg(from.toString(Qt::ISODate), to.toString(Qt::ISODate));
    }
    if (m_windowDays == 1) {
        return QStringLiteral("24h");
    }
    if (m_windowDays == 2) {
        return QStringLiteral("48h");
    }
    if (m_windowDays == 7) {
        return QStringLiteral("1 week");
    }
    if (m_windowDays == 14) {
        return QStringLiteral("2 weeks");
    }
    return QStringLiteral("%1d").arg(m_windowDays);
}

void DashboardViewModel::setSelectedStoreId(const QString& storeId)
{
    const QString trimmed = storeId.trimmed();
    if (m_selectedStoreId == trimmed) {
        return;
    }
    m_selectedStoreId = trimmed;
    emit selectedStoreIdChanged();
    refresh();
}

void DashboardViewModel::setWindowDays(int days)
{
    const int clamped = std::max(kMinWindowDays, std::min(days, kMaxWindowDays));
    if (m_windowDays == clamped && !m_customRange) {
        return;
    }
    m_windowDays = clamped;
    m_customRange = false;
    const QDate today = QDate::currentDate();
    m_customFrom = today.addDays(-(m_windowDays - 1)).toString(Qt::ISODate);
    m_customTo = today.toString(Qt::ISODate);
    emit windowDaysChanged();
    emit rangeChanged();
    refresh();
}

void DashboardViewModel::setCustomRange(bool enabled)
{
    if (m_customRange == enabled) {
        return;
    }
    m_customRange = enabled;
    emit rangeChanged();
    refresh();
}

void DashboardViewModel::setCustomFrom(const QString& isoDate)
{
    if (m_customFrom == isoDate) {
        return;
    }
    m_customFrom = isoDate;
    emit rangeChanged();
}

void DashboardViewModel::setCustomTo(const QString& isoDate)
{
    if (m_customTo == isoDate) {
        return;
    }
    m_customTo = isoDate;
    emit rangeChanged();
}

void DashboardViewModel::setLowRamReviewsOnly(bool enabled)
{
    if (m_lowRamReviewsOnly == enabled) {
        return;
    }
    m_lowRamReviewsOnly = enabled;
    emit lowRamReviewsOnlyChanged();
    refresh();
}

void DashboardViewModel::applyCustomRange(const QString& fromIso, const QString& toIso)
{
    QDate from = QDate::fromString(fromIso.trimmed(), Qt::ISODate);
    QDate to = QDate::fromString(toIso.trimmed(), Qt::ISODate);
    if (!from.isValid() || !to.isValid()) {
        setStatus(QStringLiteral("Custom range needs YYYY-MM-DD dates"));
        return;
    }
    if (from > to) {
        std::swap(from, to);
    }
    m_customFrom = from.toString(Qt::ISODate);
    m_customTo = to.toString(Qt::ISODate);
    m_customRange = true;
    m_windowDays = std::max(1, static_cast<int>(from.daysTo(to) + 1));
    emit windowDaysChanged();
    emit rangeChanged();
    refresh();
}

void DashboardViewModel::setStatus(const QString& status)
{
    if (m_status == status) {
        return;
    }
    m_status = status;
    emit statusChanged();
}

void DashboardViewModel::refresh()
{
    const auto [start, end] = activeWindow();

    m_apps = m_query.listAppsWithStats(start, end);
    if (m_selectedStoreId.isEmpty() && !m_apps.isEmpty()) {
        m_selectedStoreId = m_apps.first().toMap().value(QStringLiteral("storeId")).toString();
        emit selectedStoreIdChanged();
    }

    m_kpis = m_query.kpis(m_selectedStoreId, start, end);
    m_daily = m_query.dailyAcquisitions(m_selectedStoreId, start, end);
    // dailyAcquisitions is always zero-filled for the window. Only fall back when
    // the query failed entirely (should still be filled) or store has no acq rows
    // and we want installs as the plotted metric — still zero-filled.
    qint64 acqSum = 0;
    for (const QVariant& item : m_daily) {
        acqSum += item.toMap().value(QStringLiteral("quantity")).toLongLong();
    }
    if (acqSum <= 0) {
        const QVariantList installs = m_query.dailyInstalls(m_selectedStoreId, start, end);
        qint64 installSum = 0;
        for (const QVariant& item : installs) {
            installSum += item.toMap().value(QStringLiteral("quantity")).toLongLong();
        }
        if (installSum > 0) {
            m_daily = installs;
        }
    }
    m_markets = m_query.breakdown(m_selectedStoreId,
                                  QStringLiteral("acquisitions"),
                                  QStringLiteral("market"),
                                  QStringLiteral("quantity"),
                                  start,
                                  end);
    if (m_markets.isEmpty()) {
        m_markets = m_query.breakdown(m_selectedStoreId,
                                      QStringLiteral("installs"),
                                      QStringLiteral("market"),
                                      QStringLiteral("successful_install_count"),
                                      start,
                                      end);
    }
    m_devices = m_query.breakdown(m_selectedStoreId,
                                  QStringLiteral("acquisitions"),
                                  QStringLiteral("device_type"),
                                  QStringLiteral("quantity"),
                                  start,
                                  end);
    if (m_devices.isEmpty()) {
        m_devices = m_query.breakdown(m_selectedStoreId,
                                      QStringLiteral("installs"),
                                      QStringLiteral("device_type"),
                                      QStringLiteral("successful_install_count"),
                                      start,
                                      end);
    }
    m_osVersions = m_query.breakdown(m_selectedStoreId,
                                     QStringLiteral("installs"),
                                     QStringLiteral("os_version"),
                                     QStringLiteral("successful_install_count"),
                                     start,
                                     end);
    m_packages = m_query.packageVersions(m_selectedStoreId, start, end);
    m_versionHealth = m_query.versionHealth(m_selectedStoreId, start, end);
    m_architectures = m_query.architectureBreakdown(m_selectedStoreId, start, end);
    m_acquisitionTypes = m_query.breakdown(m_selectedStoreId,
                                           QStringLiteral("acquisitions"),
                                           QStringLiteral("acquisition_type"),
                                           QStringLiteral("quantity"),
                                           start,
                                           end);
    m_storeClients = m_query.breakdown(m_selectedStoreId,
                                       QStringLiteral("acquisitions"),
                                       QStringLiteral("store_client"),
                                       QStringLiteral("quantity"),
                                       start,
                                       end);
    m_ageGroups = m_query.breakdown(m_selectedStoreId,
                                    QStringLiteral("acquisitions"),
                                    QStringLiteral("age_group"),
                                    QStringLiteral("quantity"),
                                    start,
                                    end);
    m_genders = m_query.breakdown(m_selectedStoreId,
                                  QStringLiteral("acquisitions"),
                                  QStringLiteral("gender"),
                                  QStringLiteral("quantity"),
                                  start,
                                  end);
    m_channels = m_query.channelBreakdown(m_selectedStoreId, start, end);
    m_referrers = m_query.referrerBreakdown(m_selectedStoreId, start, end);
    m_campaigns = m_query.campaignBreakdown(m_selectedStoreId, start, end);
    m_ratingStars = m_query.ratingStarBreakdown(m_selectedStoreId);
    m_revenueProducts = m_query.revenueProducts(m_selectedStoreId, start, end);
    m_reviews = m_query.recentReviews(m_selectedStoreId, 12, m_lowRamReviewsOnly);
    m_topFailures = m_query.topFailures(m_selectedStoreId, start, end);
    m_failureDevices = m_query.failureDeviceModels(m_selectedStoreId, start, end);
    m_failureCpus = m_query.failureCpuFamilies(m_selectedStoreId, start, end);
    m_failureInsider = m_query.failureInsiderBreakdown(m_selectedStoreId, start, end);
    m_failureCabs = m_query.failureCabDownloads(m_selectedStoreId);

    setStatus(QStringLiteral("%1 · %2 · $%3 · views %4 · acq %5 · installs %6")
                  .arg(selectedDisplayName(),
                       rangeLabel(),
                       QString::number(m_kpis.value(QStringLiteral("grossSalesUsd")).toDouble(), 'f', 2),
                       QString::number(m_kpis.value(QStringLiteral("pageViews")).toLongLong()),
                       QString::number(m_kpis.value(QStringLiteral("totalAcquisitions")).toLongLong()),
                       QString::number(m_kpis.value(QStringLiteral("totalInstalls")).toLongLong())));
    emit dataChanged();
}

bool DashboardViewModel::addApp(const QString& storeId, const QString& displayName)
{
    const QString id = storeId.trimmed().toUpper();
    if (id.isEmpty()) {
        setStatus(QStringLiteral("Store ID required"));
        return false;
    }
    if (!m_appsRepo.ensureApp(id, displayName.trimmed())) {
        setStatus(m_appsRepo.lastError());
        return false;
    }
    setSelectedStoreId(id);
    refresh();
    setStatus(QStringLiteral("Added %1 — Sync to pull full Partner Center analytics").arg(id));
    return true;
}

QStringList DashboardViewModel::activeStoreIds() const
{
    QStringList ids;
    for (const QVariant& item : m_apps) {
        ids << item.toMap().value(QStringLiteral("storeId")).toString();
    }
    return ids;
}

QString DashboardViewModel::defaultExportPath(const QString& kind) const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDir().mkpath(dir);
    const QString stamp = QDate::currentDate().toString(QStringLiteral("yyyyMMdd"));
    return QDir(dir).filePath(QStringLiteral("storevitals-%1-%2-%3.csv")
                                  .arg(kind, m_selectedStoreId, stamp));
}

bool DashboardViewModel::writeCsv(const QString& path, const QString& csv)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        setStatus(QStringLiteral("Could not write %1").arg(path));
        return false;
    }
    QTextStream out(&file);
    out << csv;
    return true;
}

QString DashboardViewModel::exportAcquisitionsCsv(const QString& filePath)
{
    const auto [start, end] = activeWindow();
    const QString csv = m_query.exportAcquisitionsCsv(m_selectedStoreId, start, end);
    const QString path = filePath.isEmpty() ? defaultExportPath(QStringLiteral("acquisitions"))
                                            : normalizePath(filePath);
    if (!writeCsv(path, csv)) {
        return {};
    }
    setStatus(QStringLiteral("Exported acquisitions → %1").arg(path));
    return path;
}

QString DashboardViewModel::exportInstallsCsv(const QString& filePath)
{
    const auto [start, end] = activeWindow();
    const QString csv = m_query.exportInstallsCsv(m_selectedStoreId, start, end);
    const QString path = filePath.isEmpty() ? defaultExportPath(QStringLiteral("installs"))
                                            : normalizePath(filePath);
    if (!writeCsv(path, csv)) {
        return {};
    }
    setStatus(QStringLiteral("Exported installs → %1").arg(path));
    return path;
}

QString DashboardViewModel::exportFailuresCsv(const QString& filePath)
{
    const auto [start, end] = activeWindow();
    const QString csv = m_query.exportFailuresCsv(m_selectedStoreId, start, end);
    const QString path = filePath.isEmpty() ? defaultExportPath(QStringLiteral("failures"))
                                            : normalizePath(filePath);
    if (!writeCsv(path, csv)) {
        return {};
    }
    setStatus(QStringLiteral("Exported failures → %1").arg(path));
    return path;
}

} // namespace storepulse
