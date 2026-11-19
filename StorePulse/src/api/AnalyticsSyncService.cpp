#include "AnalyticsSyncService.hpp"

#include "Logger.hpp"
#include "RateLimiter.hpp"

#include <QTimer>

namespace storepulse {

AnalyticsSyncService::AnalyticsSyncService(PartnerCenterClient& client,
                                           SqliteDatabase& db,
                                           QObject* parent)
    : QObject(parent)
    , m_client(client)
    , m_insights(client.networkAccessManager())
    , m_db(db)
    , m_apps(db)
    , m_acquisitions(db)
    , m_failures(db)
    , m_extended(db)
{
    wireClient();
}

bool AnalyticsSyncService::isBusy() const { return m_busy; }
QString AnalyticsSyncService::lastError() const { return m_lastError; }

QString AnalyticsSyncService::stageName(Stage stage)
{
    switch (stage) {
    case Stage::Funnel: return QStringLiteral("funnel");
    case Stage::ChannelViews: return QStringLiteral("listing-clicks");
    case Stage::InsightsPageviews: return QStringLiteral("insights-pageviews");
    case Stage::Acquisitions: return QStringLiteral("acquisitions");
    case Stage::AppRevenue: return QStringLiteral("app-revenue");
    case Stage::IapRevenue: return QStringLiteral("iap-revenue");
    case Stage::Installs: return QStringLiteral("installs");
    case Stage::Usage: return QStringLiteral("usage");
    case Stage::Ratings: return QStringLiteral("ratings");
    case Stage::Reviews: return QStringLiteral("reviews");
    case Stage::Failures: return QStringLiteral("failurehits");
    case Stage::FailureDetails: return QStringLiteral("failuredetails");
    case Stage::FailureStacks: return QStringLiteral("stacktraces");
    case Stage::Done: return QStringLiteral("done");
    }
    return {};
}

int AnalyticsSyncService::stageIndex(Stage stage)
{
    return static_cast<int>(stage);
}

int AnalyticsSyncService::stageCount()
{
    return static_cast<int>(Stage::Done); // 13 work stages; Done is the sentinel
}

void AnalyticsSyncService::emitStageProgress()
{
    emit syncStageChanged(m_activeStoreId, stageIndex(m_stage), stageCount(), stageName(m_stage));
}

void AnalyticsSyncService::wireClient()
{
    connect(&m_client, &PartnerCenterClient::funnelFinished, this,
            [this](const QString& storeId, bool ok, const QString& error,
                   const QVector<FunnelMetricRow>& rows) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Funnel) {
                    return;
                }
                if (ok) {
                    if (!m_extended.replaceFunnel(storeId, m_windowStart, m_windowEnd, rows)) {
                        m_lastError = m_extended.lastError();
                        advanceAfterStage(false, m_lastError);
                        return;
                    }
                    m_stageRows = rows.size();
                    emit syncProgress(storeId, stageName(m_stage), m_stageRows);
                }
                advanceAfterStage(ok, error);
            });

    connect(&m_client, &PartnerCenterClient::channelPage, this,
            [this](const QString& storeId, const QVector<ChannelConversionRow>& rows, const QString&) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::ChannelViews) {
                    return;
                }
                if (!m_clearedStageWindow) {
                    (void)m_extended.clearChannelWindow(storeId, m_windowStart, m_windowEnd);
                    m_clearedStageWindow = true;
                }
                QVector<ChannelConversionRow> normalized = rows;
                for (ChannelConversionRow& row : normalized) {
                    if (row.dateUtc == QLatin1String("total")) {
                        row.dateUtc = m_windowEnd.toString(Qt::ISODate);
                    }
                }
                if (!m_extended.upsertChannelConversions(normalized)) {
                    m_lastError = m_extended.lastError();
                } else {
                    m_stageRows += normalized.size();
                    emit syncProgress(storeId, stageName(m_stage), m_stageRows);
                }
            });
    connect(&m_client, &PartnerCenterClient::channelFinished, this,
            [this](const QString& storeId, bool ok, const QString& error) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::ChannelViews) {
                    return;
                }
                advanceAfterStage(ok, error);
            });

    connect(&m_insights, &InsightsPageviewsClient::finished, this,
            [this](const QString& storeId, bool ok, const QString& error,
                   const QVector<InsightsPageviewRow>& rows) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::InsightsPageviews) {
                    return;
                }
                if (ok) {
                    (void)m_extended.clearInsightsPageviewsWindow(storeId, m_windowStart, m_windowEnd);
                    if (!m_extended.upsertInsightsPageviews(rows)) {
                        m_lastError = m_extended.lastError();
                        advanceAfterStage(false, m_lastError);
                        return;
                    }
                    m_stageRows = rows.size();
                    emit syncProgress(storeId, stageName(m_stage), m_stageRows);
                }
                // Soft-continue when cookies are missing; classic metrics still work.
                advanceAfterStage(ok || error.contains(QStringLiteral("cookie"), Qt::CaseInsensitive),
                                  error);
            });

    connect(&m_client, &PartnerCenterClient::acquisitionsPage, this,
            [this](const QString& storeId, const QVector<AcquisitionRow>& rows, const QString&) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Acquisitions) {
                    return;
                }
                if (!m_clearedStageWindow) {
                    (void)m_acquisitions.clearWindow(storeId, m_windowStart, m_windowEnd);
                    m_clearedStageWindow = true;
                }
                if (!rows.isEmpty() && !rows.first().applicationName.isEmpty()) {
                    (void)m_apps.ensureApp(storeId, rows.first().applicationName);
                }
                if (!m_acquisitions.upsertBatch(rows)) {
                    m_lastError = m_acquisitions.lastError();
                    Logger::error(m_lastError);
                } else {
                    m_stageRows += rows.size();
                    emit syncProgress(storeId, stageName(m_stage), m_stageRows);
                }
            });
    connect(&m_client, &PartnerCenterClient::acquisitionsFinished, this,
            [this](const QString& storeId, bool ok, const QString& error) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Acquisitions) {
                    return;
                }
                advanceAfterStage(ok, error);
            });

    connect(&m_client, &PartnerCenterClient::revenuePage, this,
            [this](const QString& storeId, const QVector<RevenueRow>& rows, const QString&) {
                if (!m_busy || storeId != m_activeStoreId
                    || (m_stage != Stage::AppRevenue && m_stage != Stage::IapRevenue)) {
                    return;
                }
                if (!m_clearedStageWindow) {
                    const QString source = (m_stage == Stage::AppRevenue)
                                               ? QStringLiteral("app")
                                               : QStringLiteral("iap");
                    (void)m_extended.clearRevenueWindow(storeId, m_windowStart, m_windowEnd, source);
                    m_clearedStageWindow = true;
                }
                if (!m_extended.upsertRevenue(rows)) {
                    m_lastError = m_extended.lastError();
                } else {
                    m_stageRows += rows.size();
                    emit syncProgress(storeId, stageName(m_stage), m_stageRows);
                }
            });
    connect(&m_client, &PartnerCenterClient::revenueFinished, this,
            [this](const QString& storeId, bool ok, const QString& error) {
                if (!m_busy || storeId != m_activeStoreId
                    || (m_stage != Stage::AppRevenue && m_stage != Stage::IapRevenue)) {
                    return;
                }
                advanceAfterStage(ok, error);
            });

    connect(&m_client, &PartnerCenterClient::installsPage, this,
            [this](const QString& storeId, const QVector<InstallRow>& rows, const QString&) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Installs) {
                    return;
                }
                if (!m_clearedStageWindow) {
                    (void)m_extended.clearInstallsWindow(storeId, m_windowStart, m_windowEnd);
                    m_clearedStageWindow = true;
                }
                if (!rows.isEmpty() && !rows.first().applicationName.isEmpty()) {
                    (void)m_apps.ensureApp(storeId, rows.first().applicationName);
                }
                if (!m_extended.upsertInstalls(rows)) {
                    m_lastError = m_extended.lastError();
                } else {
                    m_stageRows += rows.size();
                    emit syncProgress(storeId, stageName(m_stage), m_stageRows);
                }
            });
    connect(&m_client, &PartnerCenterClient::installsFinished, this,
            [this](const QString& storeId, bool ok, const QString& error) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Installs) {
                    return;
                }
                advanceAfterStage(ok, error);
            });

    connect(&m_client, &PartnerCenterClient::usagePage, this,
            [this](const QString& storeId, const QVector<UsageDailyRow>& rows, const QString&) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Usage) {
                    return;
                }
                if (!m_clearedStageWindow) {
                    (void)m_extended.clearUsageWindow(storeId, m_windowStart, m_windowEnd);
                    m_clearedStageWindow = true;
                }
                if (!m_extended.upsertUsage(rows)) {
                    m_lastError = m_extended.lastError();
                } else {
                    m_stageRows += rows.size();
                    emit syncProgress(storeId, stageName(m_stage), m_stageRows);
                }
            });
    connect(&m_client, &PartnerCenterClient::usageFinished, this,
            [this](const QString& storeId, bool ok, const QString& error) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Usage) {
                    return;
                }
                advanceAfterStage(ok, error);
            });

    connect(&m_client, &PartnerCenterClient::ratingsPage, this,
            [this](const QString& storeId, const QVector<RatingRow>& rows, const QString&) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Ratings) {
                    return;
                }
                if (!m_clearedStageWindow) {
                    // Ratings use a longer lookback than the main analytics window.
                    (void)m_extended.clearRatingsWindow(
                        storeId, QDate::currentDate().addYears(-3), QDate::currentDate());
                    m_clearedStageWindow = true;
                }
                if (!m_extended.upsertRatings(rows)) {
                    m_lastError = m_extended.lastError();
                } else {
                    m_stageRows += rows.size();
                    emit syncProgress(storeId, stageName(m_stage), m_stageRows);
                }
            });
    connect(&m_client, &PartnerCenterClient::ratingsFinished, this,
            [this](const QString& storeId, bool ok, const QString& error) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Ratings) {
                    return;
                }
                advanceAfterStage(ok, error);
            });

    connect(&m_client, &PartnerCenterClient::reviewsPage, this,
            [this](const QString& storeId, const QVector<ReviewRow>& rows, const QString&) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Reviews) {
                    return;
                }
                if (!m_extended.upsertReviews(rows)) {
                    m_lastError = m_extended.lastError();
                } else {
                    m_stageRows += rows.size();
                    emit syncProgress(storeId, stageName(m_stage), m_stageRows);
                }
            });
    connect(&m_client, &PartnerCenterClient::reviewsFinished, this,
            [this](const QString& storeId, bool ok, const QString& error) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Reviews) {
                    return;
                }
                advanceAfterStage(ok, error);
            });

    connect(&m_client, &PartnerCenterClient::failureHitsPage, this,
            [this](const QString& storeId, const QVector<FailureHitRow>& rows, const QString&) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Failures) {
                    return;
                }
                if (!m_failures.upsertBatch(rows)) {
                    m_lastError = m_failures.lastError();
                } else {
                    m_stageRows += rows.size();
                    emit syncProgress(storeId, stageName(m_stage), m_stageRows);
                }
                // AQEMU (and some apps) return empty failureHash from failurehits, so
                // failuredetails cannot run. Mirror rich hit dimensions into failure_details
                // for device/CPU/Insider RankLists; real CAB rows overwrite later when hashes exist.
                QVector<FailureDetailRow> detailRows;
                detailRows.reserve(rows.size());
                for (const FailureHitRow& hit : rows) {
                    FailureDetailRow d;
                    d.storeId = hit.storeId;
                    d.failureHash = hit.failureHash.isEmpty() ? QStringLiteral("_aggregate_")
                                                              : hit.failureHash;
                    d.failureName = hit.failureName.isEmpty() ? QStringLiteral("unattributed")
                                                              : hit.failureName;
                    d.dateUtc = hit.dateUtc;
                    // Synthetic cab key keeps UNIQUE rows when Microsoft omits cabId/hash.
                    d.cabId = hit.cabId.isEmpty()
                                  ? QStringLiteral("hit:%1|%2|%3|%4|%5|%6")
                                        .arg(hit.osArchitecture,
                                             hit.cpuManufacturer,
                                             hit.cpuFamily,
                                             hit.cpuName,
                                             hit.packageVersion,
                                             hit.osRelease)
                                  : hit.cabId;
                    d.market = hit.market;
                    d.deviceType = hit.deviceType;
                    d.deviceModel = hit.deviceType; // hits lack model; device type still useful
                    d.osVersion = hit.osVersion;
                    d.osRelease = hit.osRelease;
                    d.osArchitecture = hit.osArchitecture;
                    d.packageVersion = hit.packageVersion;
                    d.cpuManufacturer = hit.cpuManufacturer;
                    d.cpuFamily = hit.cpuFamily;
                    d.cpuName = hit.cpuName.isEmpty() ? hit.cpuFamily : hit.cpuName;
                    d.flightRing = hit.flightRing;
                    d.sandboxId = hit.sandboxId;
                    d.eventCount = hit.eventCount;
                    d.stackTrace = hit.stackTrace;
                    detailRows.push_back(std::move(d));
                }
                if (!detailRows.isEmpty() && !m_extended.upsertFailureDetails(detailRows)
                    && m_lastError.isEmpty()) {
                    m_lastError = m_extended.lastError();
                }
            });
    connect(&m_client, &PartnerCenterClient::failureHitsFinished, this,
            [this](const QString& storeId, bool ok, const QString& error) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::Failures) {
                    return;
                }
                advanceAfterStage(ok, error);
            });

    connect(&m_client, &PartnerCenterClient::failureDetailsPage, this,
            [this](const QString& storeId, const QVector<FailureDetailRow>& rows, const QString&) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::FailureDetails) {
                    return;
                }
                if (!m_extended.upsertFailureDetails(rows)) {
                    m_lastError = m_extended.lastError();
                } else {
                    m_stageRows += rows.size();
                    emit syncProgress(storeId, stageName(m_stage), m_stageRows);
                }
            });
    connect(&m_client, &PartnerCenterClient::failureDetailsFinished, this,
            [this](const QString& storeId, bool ok, const QString& error) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::FailureDetails) {
                    return;
                }
                if (!ok) {
                    // Soft-continue across individual hashes (404/empty/rate-limit soft).
                    const bool hard = error.contains(QStringLiteral("401"))
                                      || error.contains(QStringLiteral("403"));
                    if (hard) {
                        advanceAfterStage(false, error);
                        return;
                    }
                    Logger::warning(QStringLiteral("failuredetails soft-fail: %1").arg(error));
                }
                if (m_detailHashQueue.isEmpty()) {
                    advanceAfterStage(true, {});
                } else {
                    QTimer::singleShot(RateLimiter::kDefaultMinIntervalSeconds * 1000, this, [this]() {
                        fetchNextFailureDetail();
                    });
                }
            });

    connect(&m_client, &PartnerCenterClient::stackTraceFinished, this,
            [this](const QString& storeId, const QString& cabId, bool ok, const QString& stack,
                   const QString& error) {
                if (!m_busy || storeId != m_activeStoreId || m_stage != Stage::FailureStacks) {
                    return;
                }
                if (ok && !stack.isEmpty()) {
                    if (!m_extended.updateFailureDetailStack(storeId, cabId, stack)) {
                        m_lastError = m_extended.lastError();
                    } else {
                        ++m_stageRows;
                        emit syncProgress(storeId, stageName(m_stage), m_stageRows);
                    }
                } else if (!ok) {
                    Logger::warning(QStringLiteral("stacktrace soft-fail cab=%1: %2").arg(cabId, error));
                }
                if (m_stackCabQueue.isEmpty()) {
                    advanceAfterStage(true, {});
                } else {
                    QTimer::singleShot(RateLimiter::kDefaultMinIntervalSeconds * 1000, this, [this]() {
                        fetchNextStackTrace();
                    });
                }
            });
}

void AnalyticsSyncService::syncApp(const QString& storeId) { syncApps({storeId}); }

void AnalyticsSyncService::syncApps(const QStringList& storeIds)
{
    if (m_busy) {
        Logger::warning(QStringLiteral("Sync already in progress"));
        return;
    }
    m_queue.clear();
    for (const QString& id : storeIds) {
        const QString trimmed = id.trimmed();
        if (!trimmed.isEmpty()) {
            m_queue.push_back(trimmed);
        }
    }
    if (m_queue.isEmpty()) {
        emit syncFinished({}, false, QStringLiteral("No Store IDs to sync"));
        return;
    }
    m_busy = true;
    startNextInQueue();
}

void AnalyticsSyncService::startNextInQueue()
{
    if (m_queue.isEmpty()) {
        m_busy = false;
        return;
    }
    beginAppSync(m_queue.takeFirst());
}

void AnalyticsSyncService::beginAppSync(const QString& storeId)
{
    m_activeStoreId = storeId;
    m_lastError.clear();
    m_hadHardFailure = false;
    m_windowEnd = QDate::currentDate();
    m_windowStart = m_windowEnd.addDays(-(kRestatementDays - 1));
    m_healthStart = m_windowEnd.addDays(-29); // failuredetails / stacks / CABs: 30-day window
    m_detailHashQueue.clear();
    m_stackCabQueue.clear();
    (void)m_apps.ensureApp(storeId);

    Logger::info(QStringLiteral("Full Partner Center sync for %1 [%2 .. %3]")
                     .arg(storeId,
                          m_windowStart.toString(Qt::ISODate),
                          m_windowEnd.toString(Qt::ISODate)));
    runStage(Stage::Funnel);
}

void AnalyticsSyncService::runStage(Stage stage)
{
    m_stage = stage;
    m_stageRows = 0;
    m_clearedStageWindow = false;
    emit syncProgress(m_activeStoreId, stageName(stage), 0);
    emitStageProgress();

    AnalyticsQuery q;
    q.applicationId = m_activeStoreId;
    q.startDate = m_windowStart;
    q.endDate = m_windowEnd;

    switch (stage) {
    case Stage::Funnel:
        // Funnel unique-user steps (not the Insights 28K listing-view total).
        m_client.fetchFunnel(q);
        break;
    case Stage::ChannelViews:
        // Referrers + campaigns + channel type (Store Traffic / CustomCampaignId / Other).
        q.groupBy = QStringLiteral(
            "date,channelType,referrerUriDomain,customCampaignId,deviceType,market");
        q.aggregationLevel = QStringLiteral("day");
        q.orderBy = QStringLiteral("date");
        m_client.fetchChannelConversions(q);
        break;
    case Stage::InsightsPageviews:
        // Partner Center Insights overview card (~28K), cookie-auth web API.
        m_insights.fetch(m_activeStoreId, m_windowStart, m_windowEnd);
        break;
    case Stage::Acquisitions:
        // Clear only after a successful page in the finished handler path —
        // wipe here only if we already have a fresh dimensional pull queued.
        // Prefer delete-on-success via clearUndimensioned + upsert; full replace
        // happens once the first acquisitions page lands (see acquisitionsPage).
        q.groupBy = QStringLiteral(
            "date,market,deviceType,osVersion,acquisitionType,storeClient,ageGroup,gender");
        q.orderBy = QStringLiteral("date");
        m_client.fetchAppAcquisitions(q);
        break;
    case Stage::AppRevenue:
        // Minimal groupby so Partner Center returns purchasePriceUSDAmount (gross sales).
        q.groupBy = QStringLiteral("date,acquisitionType,market");
        q.aggregationLevel = QStringLiteral("day");
        q.orderBy = QStringLiteral("date");
        m_client.fetchAppRevenue(q);
        break;
    case Stage::IapRevenue:
        q.groupBy = QStringLiteral("date,acquisitionType,market");
        q.aggregationLevel = QStringLiteral("day");
        q.orderBy = QStringLiteral("date");
        m_client.fetchInAppAcquisitions(q);
        break;
    case Stage::Installs:
        q.groupBy = QStringLiteral("date,market,deviceType,osVersion,packageVersion");
        q.orderBy = QStringLiteral("date");
        m_client.fetchInstalls(q);
        break;
    case Stage::Usage: {
        // Daily usage retention is ~90 days even when the main sync window is longer.
        AnalyticsQuery usage = q;
        const QDate usageFloor = m_windowEnd.addDays(-(kUsageRetentionDays - 1));
        if (usage.startDate < usageFloor) {
            usage.startDate = usageFloor;
        }
        usage.groupBy = QStringLiteral("date,market,deviceType,packageVersion");
        usage.orderBy = QStringLiteral("date");
        m_client.fetchUsageDaily(usage);
        break;
    }
    case Stage::Ratings: {
        // Ratings are sparse; pull a long history without groupby (groupby often
        // returns empty even when plain ratings exist).
        AnalyticsQuery ratings = q;
        ratings.startDate = QDate::currentDate().addYears(-3);
        ratings.groupBy.clear();
        m_client.fetchRatings(ratings);
        break;
    }
    case Stage::Reviews: {
        AnalyticsQuery reviews = q;
        reviews.startDate = QDate::currentDate().addYears(-3);
        m_client.fetchReviews(reviews);
        break;
    }
    case Stage::Failures: {
        AnalyticsQuery health = q;
        health.startDate = m_healthStart;
        // Rich dimensions: when failureHash is blank (common), these still power
        // crash device/CPU/Insider RankLists via failure_details mirroring.
        health.groupBy = QStringLiteral(
            "date,failureName,failureHash,osVersion,osRelease,osArchitecture,eventType,"
            "market,deviceType,packageName,packageVersion,cpuManufacturer,cpuFamilyName,cpu,"
            "sandboxId");
        health.orderBy = QStringLiteral("date");
        (void)m_extended.clearFailureDetailsWindow(m_activeStoreId, m_healthStart, m_windowEnd);
        m_client.fetchFailureHits(health);
        break;
    }
    case Stage::FailureDetails:
        m_detailHashQueue = m_extended.topFailureHashes(
            m_activeStoreId, m_healthStart, m_windowEnd, kFailureDetailHashLimit);
        // Only refetch/clear when Microsoft returned real hashes; otherwise keep
        // the failurehits-mirrored detail rows from the Failures stage.
        if (m_detailHashQueue.isEmpty()) {
            advanceAfterStage(true, {});
            return;
        }
        (void)m_extended.clearFailureDetailsWindow(m_activeStoreId, m_healthStart, m_windowEnd);
        m_clearedStageWindow = true;
        fetchNextFailureDetail();
        break;
    case Stage::FailureStacks:
        m_stackCabQueue = m_extended.stackCabTargets(m_activeStoreId, kStackCabLimit);
        if (m_stackCabQueue.isEmpty()) {
            advanceAfterStage(true, {});
            return;
        }
        fetchNextStackTrace();
        break;
    case Stage::Done:
        break;
    }
}

void AnalyticsSyncService::fetchNextFailureDetail()
{
    if (m_detailHashQueue.isEmpty()) {
        advanceAfterStage(true, {});
        return;
    }
    const QString hash = m_detailHashQueue.takeFirst();
    AnalyticsQuery q;
    q.applicationId = m_activeStoreId;
    q.startDate = m_healthStart;
    q.endDate = m_windowEnd;
    q.failureHash = hash;
    q.top = 10000;
    m_client.fetchFailureDetails(q);
}

void AnalyticsSyncService::fetchNextStackTrace()
{
    if (m_stackCabQueue.isEmpty()) {
        advanceAfterStage(true, {});
        return;
    }
    const auto pair = m_stackCabQueue.takeFirst();
    AnalyticsQuery q;
    q.applicationId = m_activeStoreId;
    q.cabId = pair.second;
    m_client.fetchStackTrace(q);
}

void AnalyticsSyncService::advanceAfterStage(bool ok, const QString& error)
{
    if (!ok) {
        // Soft-continue on empty/optional datasets; hard-stop on auth/rate-limit.
        const bool hard = error.contains(QStringLiteral("401"))
                          || error.contains(QStringLiteral("403"))
                          || error.contains(QStringLiteral("429"))
                          || error.contains(QStringLiteral("rate limited"), Qt::CaseInsensitive);
        if (hard) {
            m_hadHardFailure = true;
            m_lastError = error;
            m_busy = false;
            m_stage = Stage::Done;
            emit syncStageChanged(m_activeStoreId, stageCount(), stageCount(),
                                  QStringLiteral("done"));
            emit syncFinished(m_activeStoreId, false, m_lastError);
            return;
        }
        Logger::warning(QStringLiteral("Stage %1 soft-failed: %2")
                            .arg(stageName(m_stage), error));
        if (m_lastError.isEmpty()) {
            m_lastError = error;
        }
    }

    Stage next = Stage::Done;
    switch (m_stage) {
    case Stage::Funnel: next = Stage::ChannelViews; break;
    case Stage::ChannelViews: next = Stage::InsightsPageviews; break;
    case Stage::InsightsPageviews: next = Stage::Acquisitions; break;
    case Stage::Acquisitions: next = Stage::AppRevenue; break;
    case Stage::AppRevenue: next = Stage::IapRevenue; break;
    case Stage::IapRevenue: next = Stage::Installs; break;
    case Stage::Installs: next = Stage::Usage; break;
    case Stage::Usage: next = Stage::Ratings; break;
    case Stage::Ratings: next = Stage::Reviews; break;
    case Stage::Reviews: next = Stage::Failures; break;
    case Stage::Failures: next = Stage::FailureDetails; break;
    case Stage::FailureDetails: next = Stage::FailureStacks; break;
    case Stage::FailureStacks: next = Stage::Done; break;
    case Stage::Done: next = Stage::Done; break;
    }

    if (next == Stage::Done) {
        const bool okAll = !m_hadHardFailure;
        Logger::info(QStringLiteral("Full sync finished for %1 ok=%2")
                         .arg(m_activeStoreId)
                         .arg(okAll));
        // Clear busy before syncFinished so UI listeners see isBusy()==false
        // when this was the last app in the queue.
        const bool moreQueued = !m_queue.isEmpty();
        if (!moreQueued) {
            m_busy = false;
        }
        emit syncStageChanged(m_activeStoreId, stageCount(), stageCount(),
                              QStringLiteral("done"));
        emit syncFinished(m_activeStoreId, okAll, okAll ? QString() : m_lastError);
        if (moreQueued) {
            QTimer::singleShot(RateLimiter::kDefaultMinIntervalSeconds * 1000, this, [this]() {
                startNextInQueue();
            });
        }
        return;
    }

    QTimer::singleShot(RateLimiter::kDefaultMinIntervalSeconds * 1000, this, [this, next]() {
        runStage(next);
    });
}

} // namespace storepulse
