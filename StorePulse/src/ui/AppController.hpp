#pragma once

#include "AnalyticsSyncService.hpp"
#include "AuthViewModel.hpp"
#include "DashboardViewModel.hpp"
#include "EntraAuthClient.hpp"
#include "PartnerCenterClient.hpp"
#include "SqliteDatabase.hpp"
#include "StoreLicenseManager.hpp"

#include <QNetworkAccessManager>
#include <QObject>
#include <memory>

namespace storepulse {

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(AuthViewModel* auth READ auth CONSTANT)
    Q_PROPERTY(DashboardViewModel* dashboard READ dashboard CONSTANT)
    Q_PROPERTY(StoreLicenseManager* license READ license CONSTANT)
    Q_PROPERTY(bool syncing READ syncing NOTIFY syncingChanged)
    Q_PROPERTY(double syncProgress READ syncProgress NOTIFY syncProgressChanged)
    Q_PROPERTY(QString syncStatus READ syncStatus NOTIFY syncStatusChanged)
    Q_PROPERTY(QString focusStoreId READ focusStoreId WRITE setFocusStoreId NOTIFY focusStoreIdChanged)
    Q_PROPERTY(bool hasApiCredentials READ hasApiCredentials NOTIFY apiCredentialsChanged)

public:
    AppController(SqliteDatabase& db, QNetworkAccessManager* nam, QObject* parent = nullptr);

    [[nodiscard]] AuthViewModel* auth() const { return m_auth; }
    [[nodiscard]] DashboardViewModel* dashboard() const { return m_dashboard; }
    [[nodiscard]] StoreLicenseManager* license() const { return m_license; }
    [[nodiscard]] bool syncing() const { return m_syncing; }
    [[nodiscard]] double syncProgress() const { return m_syncProgress; }
    [[nodiscard]] QString syncStatus() const { return m_syncStatus; }
    [[nodiscard]] QString focusStoreId() const { return m_focusStoreId; }
    [[nodiscard]] bool hasApiCredentials() const;
    void setFocusStoreId(const QString& storeId);

    Q_INVOKABLE void syncNow();
    Q_INVOKABLE void syncAllApps();
    Q_INVOKABLE void refreshApiCredentialState();
    Q_INVOKABLE void downloadCab(const QString& cabId);
    Q_INVOKABLE void copyText(const QString& text);
    Q_INVOKABLE void openPartnerCenterReviews(const QString& storeId = {});
    Q_INVOKABLE QString partnerCenterReviewsUrl(const QString& storeId = {}) const;
    Q_INVOKABLE void copyReplyAndOpenPartnerCenter(const QString& replyText,
                                                   const QString& storeId = {});

signals:
    void syncingChanged();
    void syncProgressChanged();
    void syncStatusChanged();
    void focusStoreIdChanged();
    void apiCredentialsChanged();

private:
    SqliteDatabase& m_db;
    QNetworkAccessManager* m_nam = nullptr;
    AuthViewModel* m_auth = nullptr;
    DashboardViewModel* m_dashboard = nullptr;
    StoreLicenseManager* m_license = nullptr;
    std::unique_ptr<EntraAuthClient> m_apiAuth;
    std::unique_ptr<PartnerCenterClient> m_client;
    std::unique_ptr<AnalyticsSyncService> m_sync;
    QString m_focusStoreId = QStringLiteral("9P0HGKHQ9W19");
    QString m_syncStatus;
    bool m_syncing = false;
    double m_syncProgress = 0.0;
    int m_syncAppsTotal = 1;
    int m_syncAppsDone = 0;

    void rebuildApiClient();
    bool beginSyncGuard();
    void setSyncProgress(double value);
    void beginSyncSession(int appCount);
};

} // namespace storepulse
