#include "AppController.hpp"

#include "Config.hpp"
#include "CredentialManager.hpp"
#include "Logger.hpp"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QtGlobal>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

namespace storepulse {

AppController::AppController(SqliteDatabase& db, QNetworkAccessManager* nam, QObject* parent)
    : QObject(parent)
    , m_db(db)
    , m_nam(nam)
    , m_auth(new AuthViewModel(nam, this))
    , m_dashboard(new DashboardViewModel(db, this))
    , m_license(new StoreLicenseManager(this))
{
    connect(m_auth, &AuthViewModel::apiCredentialsSaved, this, &AppController::refreshApiCredentialState);
    connect(m_dashboard, &DashboardViewModel::selectedStoreIdChanged, this, [this]() {
        setFocusStoreId(m_dashboard->selectedStoreId());
    });
    setFocusStoreId(m_dashboard->selectedStoreId());
    rebuildApiClient();
    m_syncStatus = hasApiCredentials()
                       ? QStringLiteral("API credentials ready — Sync pulls dimensional analytics into SQLite")
                       : QStringLiteral("Add API credentials in Settings to sync analytics.");
}

bool AppController::hasApiCredentials() const
{
    const auto& cfg = Config::instance();
    if (cfg.tenantId().trimmed().isEmpty() || cfg.clientId().trimmed().isEmpty()) {
        return false;
    }
    CredentialManager creds;
    return creds.hasSecret();
}

void AppController::refreshApiCredentialState()
{
    rebuildApiClient();
    emit apiCredentialsChanged();
    if (hasApiCredentials()) {
        m_syncStatus = QStringLiteral("API credentials saved — you can Sync now");
    } else {
        m_syncStatus = QStringLiteral("API credentials incomplete — tenant ID, client ID, and secret required");
    }
    emit syncStatusChanged();
}

void AppController::rebuildApiClient()
{
    m_sync.reset();
    m_client.reset();
    m_apiAuth.reset();

    if (hasApiCredentials()) {
        m_apiAuth = std::make_unique<EntraAuthClient>(m_nam);
        m_apiAuth->setTenantId(Config::instance().tenantId().trimmed());
        m_apiAuth->setClientId(Config::instance().clientId().trimmed());
        m_client = std::make_unique<PartnerCenterClient>(*m_apiAuth, m_nam);
        Logger::info(QStringLiteral("Sync client wired to Entra client-credentials (Dev Center)"));
    } else {
        m_client = std::make_unique<PartnerCenterClient>(*m_auth->client(), m_nam);
        Logger::info(QStringLiteral("No API credentials - sync blocked until Settings are completed"));
    }

    m_sync = std::make_unique<AnalyticsSyncService>(*m_client, m_db);
    connect(m_sync.get(), &AnalyticsSyncService::syncProgress, this,
            [this](const QString&, const QString& stage, int rows) {
                m_syncStatus = QStringLiteral("%1 — %2 rows").arg(stage).arg(rows);
                emit syncStatusChanged();
            });
    connect(m_sync.get(), &AnalyticsSyncService::syncStageChanged, this,
            [this](const QString&, int stageIndex, int stageCount, const QString& stageName) {
                const int total = qMax(1, m_syncAppsTotal);
                double stageFrac = 1.0;
                if (stageCount > 0 && stageName != QLatin1String("done")
                    && stageIndex < stageCount) {
                    // Entering stage i of N → show (i+1)/N so the bar moves immediately.
                    stageFrac = double(stageIndex + 1) / double(stageCount);
                }
                setSyncProgress((double(m_syncAppsDone) + stageFrac) / double(total));
                if (!stageName.isEmpty() && stageName != QLatin1String("done")) {
                    m_syncStatus = QStringLiteral("%1 (%2/%3)")
                                       .arg(stageName)
                                       .arg(qMin(stageIndex + 1, stageCount))
                                       .arg(stageCount);
                    emit syncStatusChanged();
                }
            });
    connect(m_sync.get(), &AnalyticsSyncService::syncFinished, this,
            [this](const QString& storeId, bool ok, const QString& error) {
                ++m_syncAppsDone;
                const bool stillBusy = m_sync && m_sync->isBusy();
                if (!stillBusy) {
                    setSyncProgress(1.0);
                    m_syncing = false;
                    emit syncingChanged();
                    QTimer::singleShot(800, this, [this]() {
                        if (!m_syncing) {
                            setSyncProgress(0.0);
                        }
                    });
                } else {
                    const int total = qMax(1, m_syncAppsTotal);
                    setSyncProgress(double(m_syncAppsDone) / double(total));
                }
                m_syncStatus = ok ? QStringLiteral("Sync complete for %1").arg(storeId)
                                  : QStringLiteral("Sync failed: %1").arg(error);
                emit syncStatusChanged();
                m_dashboard->refresh();
            });
    connect(m_client.get(), &PartnerCenterClient::cabDownloadFinished, this,
            [this](const QString&, const QString& cabId, bool ok, const QString& path,
                   const QString& error) {
                m_syncStatus = ok ? QStringLiteral("CAB saved → %1").arg(path)
                                  : QStringLiteral("CAB %1 failed: %2").arg(cabId, error);
                emit syncStatusChanged();
            });
}

void AppController::setFocusStoreId(const QString& storeId)
{
    if (m_focusStoreId == storeId) {
        return;
    }
    m_focusStoreId = storeId;
    emit focusStoreIdChanged();
    if (m_dashboard && m_dashboard->selectedStoreId() != storeId) {
        m_dashboard->setSelectedStoreId(storeId);
    }
}

bool AppController::beginSyncGuard()
{
    if (m_syncing) {
        return false;
    }
    if (!hasApiCredentials()) {
        m_syncStatus = QStringLiteral(
            "Microsoft Store analytics needs Dev Center API credentials in Settings "
            "(tenant + client ID + secret) after linking the app as Manager.");
        emit syncStatusChanged();
        return false;
    }
    return true;
}

void AppController::setSyncProgress(double value)
{
    const double clamped = qBound(0.0, value, 1.0);
    if (qFuzzyCompare(m_syncProgress, clamped)) {
        return;
    }
    m_syncProgress = clamped;
    emit syncProgressChanged();
}

void AppController::beginSyncSession(int appCount)
{
    m_syncAppsTotal = qMax(1, appCount);
    m_syncAppsDone = 0;
    setSyncProgress(0.0);
    m_syncing = true;
    emit syncingChanged();
}

void AppController::syncNow()
{
    if (!beginSyncGuard()) {
        return;
    }

    beginSyncSession(1);
    m_syncStatus = QStringLiteral("Syncing %1...").arg(m_focusStoreId);
    emit syncStatusChanged();
    Logger::info(m_syncStatus);
    m_sync->syncApp(m_focusStoreId);
}

void AppController::syncAllApps()
{
    if (!beginSyncGuard()) {
        return;
    }

    const QStringList ids = m_dashboard->activeStoreIds();
    if (ids.isEmpty()) {
        m_syncStatus = QStringLiteral("Add at least one app before Sync all");
        emit syncStatusChanged();
        return;
    }

    beginSyncSession(ids.size());
    m_syncStatus = QStringLiteral("Syncing %1 apps...").arg(ids.size());
    emit syncStatusChanged();
    Logger::info(m_syncStatus);
    m_sync->syncApps(ids);
}

void AppController::downloadCab(const QString& cabId)
{
    if (!m_client || cabId.trimmed().isEmpty()) {
        m_syncStatus = QStringLiteral("CAB download needs API credentials and a cabId");
        emit syncStatusChanged();
        return;
    }
    if (!hasApiCredentials()) {
        beginSyncGuard();
        return;
    }
    const QString dir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                            .filePath(QStringLiteral("cabs"));
    QDir().mkpath(dir);
    const QString path = QDir(dir).filePath(QStringLiteral("%1-%2.cab")
                                                .arg(m_focusStoreId, cabId.trimmed()));
    m_syncStatus = QStringLiteral("Downloading CAB %1…").arg(cabId);
    emit syncStatusChanged();
    m_client->downloadCab(m_focusStoreId, cabId.trimmed(), path);
}

void AppController::copyText(const QString& text)
{
    if (QClipboard* clip = QGuiApplication::clipboard()) {
        clip->setText(text);
        m_syncStatus = QStringLiteral("Copied to clipboard");
        emit syncStatusChanged();
    }
}

QString AppController::partnerCenterReviewsUrl(const QString& storeId) const
{
    const QString id = storeId.trimmed().isEmpty() ? m_focusStoreId : storeId.trimmed();
    return QStringLiteral("https://partner.microsoft.com/dashboard/products/%1/reviews").arg(id);
}

void AppController::openPartnerCenterReviews(const QString& storeId)
{
    const QUrl url(partnerCenterReviewsUrl(storeId));
    if (!QDesktopServices::openUrl(url)) {
        m_syncStatus = QStringLiteral("Could not open Partner Center reviews");
        emit syncStatusChanged();
        return;
    }
    m_syncStatus = QStringLiteral("Opened Partner Center reviews");
    emit syncStatusChanged();
}

void AppController::copyReplyAndOpenPartnerCenter(const QString& replyText, const QString& storeId)
{
    copyText(replyText);
    openPartnerCenterReviews(storeId);
    m_syncStatus = QStringLiteral("Reply copied — paste it in Partner Center");
    emit syncStatusChanged();
}

} // namespace storepulse
