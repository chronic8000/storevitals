#pragma once

#include "Models.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

namespace storepulse {

struct PagedPayload {
    QVector<QJsonObject> values;
    QString nextLink;
};

namespace AnalyticsParsers {

[[nodiscard]] PagedPayload parsePage(const QByteArray& json);

[[nodiscard]] QVector<AcquisitionRow> parseAcquisitions(const QByteArray& json,
                                                        const QString& storeId,
                                                        const QString& source = QStringLiteral("analytics"));

[[nodiscard]] QVector<AcquisitionRow> parseAcquisitionObjects(const QVector<QJsonObject>& values,
                                                              const QString& storeId,
                                                              const QString& source = QStringLiteral("analytics"));

[[nodiscard]] QVector<RevenueRow> parseRevenueObjects(const QVector<QJsonObject>& values,
                                                      const QString& storeId,
                                                      const QString& source);

[[nodiscard]] QVector<FailureHitRow> parseFailureHits(const QByteArray& json, const QString& storeId);

[[nodiscard]] QVector<FailureHitRow> parseFailureHitObjects(const QVector<QJsonObject>& values,
                                                            const QString& storeId);

[[nodiscard]] QVector<FailureDetailRow> parseFailureDetailObjects(const QVector<QJsonObject>& values,
                                                                  const QString& storeId);

[[nodiscard]] QString formatStackTrace(const QVector<QJsonObject>& frames);

[[nodiscard]] QVector<FunnelMetricRow> parseFunnel(const QByteArray& json,
                                                   const QString& storeId,
                                                   const QString& windowStart,
                                                   const QString& windowEnd);

[[nodiscard]] QVector<ChannelConversionRow> parseChannelObjects(const QVector<QJsonObject>& values,
                                                                const QString& storeId);

[[nodiscard]] QVector<InstallRow> parseInstallObjects(const QVector<QJsonObject>& values,
                                                      const QString& storeId);
[[nodiscard]] QVector<UsageDailyRow> parseUsageObjects(const QVector<QJsonObject>& values,
                                                       const QString& storeId);
[[nodiscard]] QVector<RatingRow> parseRatingObjects(const QVector<QJsonObject>& values,
                                                    const QString& storeId);
[[nodiscard]] QVector<ReviewRow> parseReviewObjects(const QVector<QJsonObject>& values,
                                                    const QString& storeId);

[[nodiscard]] QString jsonString(const QJsonObject& obj, const QStringList& keys);
[[nodiscard]] qint64 jsonInt(const QJsonObject& obj, const QStringList& keys, qint64 fallback = 0);

} // namespace AnalyticsParsers
} // namespace storepulse
