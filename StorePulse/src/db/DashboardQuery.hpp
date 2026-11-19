#pragma once

#include "SqliteDatabase.hpp"

#include <QDate>
#include <QVariantList>
#include <QVariantMap>

namespace storepulse {

class DashboardQuery {
public:
    explicit DashboardQuery(SqliteDatabase& db);

    [[nodiscard]] QVariantList listAppsWithStats(const QDate& fromUtc, const QDate& toUtc) const;
    [[nodiscard]] QVariantMap kpis(const QString& storeId, const QDate& fromUtc, const QDate& toUtc) const;
    [[nodiscard]] QVariantMap funnel(const QString& storeId, const QDate& fromUtc, const QDate& toUtc) const;
    [[nodiscard]] QVariantList dailyAcquisitions(const QString& storeId,
                                                const QDate& fromUtc,
                                                const QDate& toUtc) const;
    [[nodiscard]] QVariantList dailyInstalls(const QString& storeId,
                                            const QDate& fromUtc,
                                            const QDate& toUtc) const;
    [[nodiscard]] QVariantList breakdown(const QString& storeId,
                                        const QString& table,
                                        const QString& column,
                                        const QString& valueColumn,
                                        const QDate& fromUtc,
                                        const QDate& toUtc,
                                        int limit = 10) const;
    [[nodiscard]] QVariantList packageVersions(const QString& storeId,
                                               const QDate& fromUtc,
                                               const QDate& toUtc) const;
    /// Per package version: installs + DAU sum + crash events (flags thin releases).
    [[nodiscard]] QVariantList versionHealth(const QString& storeId,
                                             const QDate& fromUtc,
                                             const QDate& toUtc,
                                             int limit = 10) const;
    [[nodiscard]] QVariantList architectureBreakdown(const QString& storeId,
                                                     const QDate& fromUtc,
                                                     const QDate& toUtc) const;
    [[nodiscard]] QVariantList channelBreakdown(const QString& storeId,
                                                const QDate& fromUtc,
                                                const QDate& toUtc,
                                                int limit = 10) const;
    [[nodiscard]] QVariantList referrerBreakdown(const QString& storeId,
                                                 const QDate& fromUtc,
                                                 const QDate& toUtc,
                                                 int limit = 10) const;
    [[nodiscard]] QVariantList campaignBreakdown(const QString& storeId,
                                                 const QDate& fromUtc,
                                                 const QDate& toUtc,
                                                 int limit = 10) const;
    [[nodiscard]] QVariantList ratingStarBreakdown(const QString& storeId) const;
    [[nodiscard]] QVariantList revenueProducts(const QString& storeId,
                                               const QDate& fromUtc,
                                               const QDate& toUtc,
                                               int limit = 10) const;
    [[nodiscard]] QVariantList recentReviews(const QString& storeId,
                                             int limit = 12,
                                             bool lowRamOnly = false) const;
    [[nodiscard]] QVariantList topFailures(const QString& storeId,
                                          const QDate& fromUtc,
                                          const QDate& toUtc,
                                          int limit = 8) const;
    [[nodiscard]] QVariantList failureDeviceModels(const QString& storeId,
                                                   const QDate& fromUtc,
                                                   const QDate& toUtc,
                                                   int limit = 10) const;
    [[nodiscard]] QVariantList failureCpuFamilies(const QString& storeId,
                                                  const QDate& fromUtc,
                                                  const QDate& toUtc,
                                                  int limit = 10) const;
    [[nodiscard]] QVariantList failureInsiderBreakdown(const QString& storeId,
                                                       const QDate& fromUtc,
                                                       const QDate& toUtc) const;
    [[nodiscard]] QVariantList failureCabDownloads(const QString& storeId, int limit = 8) const;
    [[nodiscard]] QString exportAcquisitionsCsv(const QString& storeId,
                                               const QDate& fromUtc,
                                               const QDate& toUtc) const;
    [[nodiscard]] QString exportInstallsCsv(const QString& storeId,
                                           const QDate& fromUtc,
                                           const QDate& toUtc) const;
    [[nodiscard]] QString exportFailuresCsv(const QString& storeId,
                                           const QDate& fromUtc,
                                           const QDate& toUtc) const;
    [[nodiscard]] QString lastError() const;

private:
    SqliteDatabase& m_db;
    mutable QString m_lastError;
};

} // namespace storepulse
