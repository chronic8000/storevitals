#pragma once

#include "Models.hpp"
#include "SqliteDatabase.hpp"

#include <QDate>
#include <QPair>
#include <QStringList>
#include <QVector>

namespace storepulse {

class ExtendedAnalyticsRepository {
public:
    explicit ExtendedAnalyticsRepository(SqliteDatabase& db);

    [[nodiscard]] bool replaceFunnel(const QString& storeId,
                                     const QDate& fromUtc,
                                     const QDate& toUtc,
                                     const QVector<FunnelMetricRow>& rows);
    [[nodiscard]] bool replaceInstallsWindow(const QString& storeId,
                                             const QDate& fromUtc,
                                             const QDate& toUtc,
                                             const QVector<InstallRow>& rows);
    [[nodiscard]] bool upsertInstalls(const QVector<InstallRow>& rows);
    [[nodiscard]] bool clearInstallsWindow(const QString& storeId, const QDate& fromUtc, const QDate& toUtc);

    [[nodiscard]] bool clearUsageWindow(const QString& storeId, const QDate& fromUtc, const QDate& toUtc);
    [[nodiscard]] bool upsertUsage(const QVector<UsageDailyRow>& rows);

    [[nodiscard]] bool clearRatingsWindow(const QString& storeId, const QDate& fromUtc, const QDate& toUtc);
    [[nodiscard]] bool upsertRatings(const QVector<RatingRow>& rows);

    [[nodiscard]] bool upsertReviews(const QVector<ReviewRow>& rows);

    [[nodiscard]] bool clearChannelWindow(const QString& storeId, const QDate& fromUtc, const QDate& toUtc);
    [[nodiscard]] bool upsertChannelConversions(const QVector<ChannelConversionRow>& rows);

    [[nodiscard]] bool clearInsightsPageviewsWindow(const QString& storeId,
                                                    const QDate& fromUtc,
                                                    const QDate& toUtc);
    [[nodiscard]] bool upsertInsightsPageviews(const QVector<InsightsPageviewRow>& rows);

    [[nodiscard]] bool clearRevenueWindow(const QString& storeId,
                                          const QDate& fromUtc,
                                          const QDate& toUtc,
                                          const QString& source = {});
    [[nodiscard]] bool upsertRevenue(const QVector<RevenueRow>& rows);

    [[nodiscard]] bool clearFailureDetailsWindow(const QString& storeId,
                                                 const QDate& fromUtc,
                                                 const QDate& toUtc);
    [[nodiscard]] bool upsertFailureDetails(const QVector<FailureDetailRow>& rows);
    [[nodiscard]] bool updateFailureDetailStack(const QString& storeId,
                                                const QString& cabId,
                                                const QString& stackTrace);
    [[nodiscard]] QStringList topFailureHashes(const QString& storeId,
                                               const QDate& fromUtc,
                                               const QDate& toUtc,
                                               int limit = 20) const;
    [[nodiscard]] QVector<QPair<QString, QString>> stackCabTargets(const QString& storeId,
                                                                   int limit = 20) const;

    [[nodiscard]] QString lastError() const;

private:
    SqliteDatabase& m_db;
    mutable QString m_lastError;
};

} // namespace storepulse
