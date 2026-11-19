#pragma once

#include "AuthProvider.hpp"
#include "Models.hpp"
#include "RateLimiter.hpp"

#include <QDate>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVector>
#include <optional>

namespace storepulse {

struct AnalyticsQuery {
    QString applicationId;
    QDate startDate;
    QDate endDate;
    int top = 10000;
    int skip = 0;
    QString groupBy;
    QString orderBy;
    QString filter;
    QString aggregationLevel; // day|week|month
    QString failureHash;      // failuredetails
    QString cabId;            // stacktrace / cabdownload
};

class PartnerCenterClient : public QObject {
    Q_OBJECT
public:
    static constexpr auto kAnalyticsBase =
        "https://manage.devcenter.microsoft.com/v1.0/my/analytics";

    explicit PartnerCenterClient(AuthProvider& auth,
                                 QNetworkAccessManager* nam,
                                 QObject* parent = nullptr);

    void setApiBaseOverride(const QUrl& base);
    [[nodiscard]] RateLimiter& rateLimiter();
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] QNetworkAccessManager* networkAccessManager() const { return m_nam; }

    void fetchAppAcquisitions(const AnalyticsQuery& query);
    void fetchAppRevenue(const AnalyticsQuery& query);
    void fetchInAppAcquisitions(const AnalyticsQuery& query);
    void fetchFunnel(const AnalyticsQuery& query);
    void fetchChannelConversions(const AnalyticsQuery& query);
    void fetchInstalls(const AnalyticsQuery& query);
    void fetchUsageDaily(const AnalyticsQuery& query);
    void fetchRatings(const AnalyticsQuery& query);
    void fetchReviews(const AnalyticsQuery& query);
    void fetchFailureHits(const AnalyticsQuery& query);
    void fetchFailureDetails(const AnalyticsQuery& query);
    void fetchStackTrace(const AnalyticsQuery& query);
    void downloadCab(const QString& storeId, const QString& cabId, const QString& destPath);

signals:
    void acquisitionsPage(const QString& storeId, const QVector<AcquisitionRow>& rows, const QString& nextLink);
    void acquisitionsFinished(const QString& storeId, bool ok, const QString& error);

    void revenuePage(const QString& storeId, const QVector<RevenueRow>& rows, const QString& nextLink);
    void revenueFinished(const QString& storeId, bool ok, const QString& error);

    void funnelFinished(const QString& storeId, bool ok, const QString& error,
                        const QVector<FunnelMetricRow>& rows);

    void channelPage(const QString& storeId, const QVector<ChannelConversionRow>& rows, const QString& nextLink);
    void channelFinished(const QString& storeId, bool ok, const QString& error);

    void installsPage(const QString& storeId, const QVector<InstallRow>& rows, const QString& nextLink);
    void installsFinished(const QString& storeId, bool ok, const QString& error);

    void usagePage(const QString& storeId, const QVector<UsageDailyRow>& rows, const QString& nextLink);
    void usageFinished(const QString& storeId, bool ok, const QString& error);

    void ratingsPage(const QString& storeId, const QVector<RatingRow>& rows, const QString& nextLink);
    void ratingsFinished(const QString& storeId, bool ok, const QString& error);

    void reviewsPage(const QString& storeId, const QVector<ReviewRow>& rows, const QString& nextLink);
    void reviewsFinished(const QString& storeId, bool ok, const QString& error);

    void failureHitsPage(const QString& storeId, const QVector<FailureHitRow>& rows, const QString& nextLink);
    void failureHitsFinished(const QString& storeId, bool ok, const QString& error);

    void failureDetailsPage(const QString& storeId, const QVector<FailureDetailRow>& rows, const QString& nextLink);
    void failureDetailsFinished(const QString& storeId, bool ok, const QString& error);

    void stackTraceFinished(const QString& storeId, const QString& cabId, bool ok,
                            const QString& stackTrace, const QString& error);

    void cabDownloadFinished(const QString& storeId, const QString& cabId, bool ok,
                             const QString& savedPath, const QString& error);

private:
    enum class Endpoint {
        AppAcquisitions,
        AppRevenue,
        InAppAcquisitions,
        Funnel,
        ChannelConversions,
        Installs,
        UsageDaily,
        Ratings,
        Reviews,
        FailureHits,
        FailureDetails,
        StackTrace,
        CabDownload
    };

    struct PendingFetch {
        Endpoint endpoint = Endpoint::AppAcquisitions;
        AnalyticsQuery query;
        QString nextUrl;
        bool followPagination = true;
        QString destPath; // cab download
    };

    AuthProvider& m_auth;
    QNetworkAccessManager* m_nam = nullptr;
    RateLimiter m_rateLimiter;
    QUrl m_baseOverride;
    QString m_lastError;
    std::optional<PendingFetch> m_pending;
    bool m_authHooksConnected = false;
    QTimer m_paceTimer;

    void ensureAuthHooks();
    void startAuthorizedFetch(PendingFetch pending);
    void continueAuthorizedFetch();
    void onTokenReady(const QString& token, const QDateTime& expiresAtUtc);
    void onTokenFailed(const QString& error);
    void onAnalyticsReply(QNetworkReply* reply, Endpoint endpoint, QString storeId, bool follow);
    void onCabReply(QNetworkReply* reply, QString storeId, QString cabId, QString destPath);
    void emitFinished(Endpoint endpoint, const QString& storeId, bool ok, const QString& error);
    [[nodiscard]] QUrl buildUrl(Endpoint endpoint, const AnalyticsQuery& query) const;
    [[nodiscard]] QString endpointPath(Endpoint endpoint) const;
};

} // namespace storepulse
