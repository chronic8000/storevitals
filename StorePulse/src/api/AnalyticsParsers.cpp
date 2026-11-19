#include "AnalyticsParsers.hpp"

#include <QDate>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonValue>

namespace storepulse {
namespace AnalyticsParsers {
namespace {

QString pickString(const QJsonObject& obj, std::initializer_list<const char*> keys)
{
    for (const char* key : keys) {
        const QJsonValue v = obj.value(QLatin1String(key));
        if (v.isString() && !v.toString().isEmpty()) {
            return v.toString();
        }
        if (v.isDouble()) {
            return QString::number(v.toVariant().toLongLong());
        }
    }
    return {};
}

qint64 pickInt(const QJsonObject& obj, std::initializer_list<const char*> keys, qint64 fallback)
{
    for (const char* key : keys) {
        const QJsonValue v = obj.value(QLatin1String(key));
        if (v.isDouble()) {
            return v.toVariant().toLongLong();
        }
        if (v.isString()) {
            bool ok = false;
            const qint64 n = v.toString().toLongLong(&ok);
            if (ok) {
                return n;
            }
        }
    }
    return fallback;
}

double pickDouble(const QJsonObject& obj, std::initializer_list<const char*> keys, double fallback = 0)
{
    for (const char* key : keys) {
        const QJsonValue v = obj.value(QLatin1String(key));
        if (v.isDouble()) {
            return v.toDouble();
        }
        if (v.isString()) {
            bool ok = false;
            const double n = v.toString().toDouble(&ok);
            if (ok) {
                return n;
            }
        }
    }
    return fallback;
}

QString normalizeDate(const QString& raw)
{
    if (raw.isEmpty()) {
        return raw;
    }
    // Keep date portion for daily rows; preserve hourly timestamps when present.
    if (raw.size() >= 10 && raw[4] == QLatin1Char('-') && raw[7] == QLatin1Char('-')) {
        if (raw.contains(QLatin1Char('T'))) {
            return raw;
        }
        return raw.left(10);
    }
    // Partner Center reviews often return US-style timestamps: "7/29/2026 12:06:54 AM"
    const QDateTime dt = QDateTime::fromString(raw, QStringLiteral("M/d/yyyy h:mm:ss AP"));
    if (dt.isValid()) {
        return dt.date().toString(Qt::ISODate);
    }
    const QDate d = QDate::fromString(raw.left(10), QStringLiteral("M/d/yyyy"));
    if (d.isValid()) {
        return d.toString(Qt::ISODate);
    }
    return raw;
}

QString architectureFromPackageName(const QString& packageName)
{
    const QString lower = packageName.toLower();
    if (lower.contains(QLatin1String("_arm64_"))) {
        return QStringLiteral("arm64");
    }
    if (lower.contains(QLatin1String("_x64_"))) {
        return QStringLiteral("x64");
    }
    if (lower.contains(QLatin1String("_x86_"))) {
        return QStringLiteral("x86");
    }
    if (lower.contains(QLatin1String("_neutral_"))) {
        return QStringLiteral("neutral");
    }
    return {};
}

} // namespace

QString jsonString(const QJsonObject& obj, const QStringList& keys)
{
    for (const QString& key : keys) {
        const QJsonValue v = obj.value(key);
        if (v.isString() && !v.toString().isEmpty()) {
            return v.toString();
        }
    }
    return {};
}

qint64 jsonInt(const QJsonObject& obj, const QStringList& keys, qint64 fallback)
{
    for (const QString& key : keys) {
        const QJsonValue v = obj.value(key);
        if (v.isDouble()) {
            return v.toVariant().toLongLong();
        }
    }
    return fallback;
}

PagedPayload parsePage(const QByteArray& json)
{
    PagedPayload page;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) {
        return page;
    }
    const QJsonObject root = doc.object();

    QJsonArray arr;
    if (root.value(QStringLiteral("Value")).isArray()) {
        arr = root.value(QStringLiteral("Value")).toArray();
    } else if (root.value(QStringLiteral("value")).isArray()) {
        arr = root.value(QStringLiteral("value")).toArray();
    }

    page.values.reserve(arr.size());
    for (const QJsonValue& v : arr) {
        if (v.isObject()) {
            page.values.push_back(v.toObject());
        }
    }

    page.nextLink = pickString(root, {"@nextLink", "@odata.nextLink", "nextLink", "NextLink"});
    return page;
}

QVector<AcquisitionRow> parseAcquisitionObjects(const QVector<QJsonObject>& values,
                                                 const QString& storeId,
                                                 const QString& source)
{
    QVector<AcquisitionRow> rows;
    rows.reserve(values.size());

    for (const QJsonObject& obj : values) {
        AcquisitionRow row;
        row.storeId = storeId;
        row.dateUtc = normalizeDate(pickString(obj, {"date", "Date", "acquisitionDate", "DateUtc"}));
        row.granularity = QStringLiteral("daily");
        row.acquisitionType = pickString(obj, {"acquisitionType", "AcquisitionType", "productTypeName"});
        row.market = pickString(obj, {"market", "Market", "countryCode"});
        row.deviceType = pickString(obj, {"deviceType", "DeviceType"});
        row.osVersion = pickString(obj, {"osVersion", "OsVersion"});
        row.ageGroup = pickString(obj, {"ageGroup", "AgeGroup"});
        row.gender = pickString(obj, {"gender", "Gender"});
        row.storeClient = pickString(obj, {"storeClient", "StoreClient", "clientType"});
        row.channel = pickString(obj, {"channel", "Channel"});
        row.campaignId = pickString(obj, {"campaignId", "CampaignId"});
        row.applicationName = pickString(obj, {"applicationName", "ApplicationName"});
        row.quantity = pickInt(obj, {"acquisitionQuantity", "AcquisitionQuantity", "quantity", "count"}, 0);
        row.purchasePriceUsd = pickDouble(obj, {"purchasePriceUSDAmount", "PurchasePriceUSDAmount",
                                                "purchasePriceUsd", "grossSalesUSD"});
        row.purchaseTaxUsd = pickDouble(obj, {"purchaseTaxUSDAmount", "PurchaseTaxUSDAmount",
                                              "purchaseTaxUsd"});
        row.source = source;
        if (!row.dateUtc.isEmpty()) {
            rows.push_back(std::move(row));
        }
    }
    return rows;
}

QVector<AcquisitionRow> parseAcquisitions(const QByteArray& json,
                                          const QString& storeId,
                                          const QString& source)
{
    return parseAcquisitionObjects(parsePage(json).values, storeId, source);
}

QVector<RevenueRow> parseRevenueObjects(const QVector<QJsonObject>& values,
                                        const QString& storeId,
                                        const QString& source)
{
    QVector<RevenueRow> rows;
    rows.reserve(values.size());
    for (const QJsonObject& obj : values) {
        RevenueRow row;
        row.storeId = storeId;
        row.dateUtc = normalizeDate(pickString(obj, {"date", "Date", "acquisitionDate"}));
        row.source = source;
        row.productId = pickString(obj, {"inAppProductId", "addonProductId", "AddonProductId",
                                         "applicationId", "ApplicationId"});
        row.productName = pickString(obj, {"inAppProductName", "InAppProductName",
                                           "applicationName", "ApplicationName", "skuDisplayName"});
        row.acquisitionType = pickString(obj, {"acquisitionType", "AcquisitionType"});
        row.market = pickString(obj, {"market", "Market"});
        row.quantity = pickInt(obj, {"acquisitionQuantity", "AcquisitionQuantity", "quantity"}, 0);
        row.purchasePriceUsd = pickDouble(obj, {"purchasePriceUSDAmount", "PurchasePriceUSDAmount",
                                                "purchasePriceUsd", "grossSalesUSD"});
        row.purchaseTaxUsd = pickDouble(obj, {"purchaseTaxUSDAmount", "PurchaseTaxUSDAmount",
                                              "purchaseTaxUsd"});
        if (!row.dateUtc.isEmpty()) {
            rows.push_back(std::move(row));
        }
    }
    return rows;
}

QVector<FailureHitRow> parseFailureHitObjects(const QVector<QJsonObject>& values,
                                              const QString& storeId)
{
    QVector<FailureHitRow> rows;
    rows.reserve(values.size());

    for (const QJsonObject& obj : values) {
        FailureHitRow row;
        row.storeId = storeId;
        row.dateUtc = normalizeDate(pickString(obj, {"date", "Date", "eventDate", "failureDate"}));
        row.failureName = pickString(obj, {"failureName", "FailureName", "name"});
        row.failureHash = pickString(obj, {"failureHash", "FailureHash", "hash"});
        row.failureStackHash = pickString(obj, {"failureStackHash", "FailureStackHash", "stackHash"});
        row.osVersion = pickString(obj, {"osVersion", "OsVersion", "operatingSystemVersion"});
        row.osRelease = pickString(obj, {"osRelease", "OsRelease"});
        row.osArchitecture = pickString(obj, {"osArchitecture", "OsArchitecture", "architecture"});
        row.packageName = pickString(obj, {"packageName", "PackageName"});
        row.packageVersion = pickString(obj, {"packageVersion", "PackageVersion", "applicationVersion"});
        if (row.osArchitecture.isEmpty()) {
            row.osArchitecture = architectureFromPackageName(row.packageName);
        }
        row.eventType = pickString(obj, {"eventType", "EventType", "failureType"});
        row.market = pickString(obj, {"market", "Market"});
        row.deviceType = pickString(obj, {"deviceType", "DeviceType"});
        row.cpuManufacturer = pickString(obj, {"cpuManufacturer", "CpuManufacturer"});
        row.cpuFamily = pickString(obj, {"cpuFamilyName", "CpuFamilyName", "cpuFamily"});
        // API often returns cpu as MHz integer; keep family/manufacturer for labels.
        if (obj.contains(QStringLiteral("cpu")) && obj.value(QStringLiteral("cpu")).isString()) {
            row.cpuName = obj.value(QStringLiteral("cpu")).toString();
        } else if (obj.contains(QStringLiteral("cpuName"))) {
            row.cpuName = pickString(obj, {"cpuName", "CpuName"});
        }
        row.sandboxId = pickString(obj, {"sandboxId", "SandboxId"});
        row.flightRing = pickString(obj, {"flightRing", "FlightRing"});
        if (row.flightRing.isEmpty() && !row.sandboxId.isEmpty()
            && row.sandboxId.compare(QLatin1String("retail"), Qt::CaseInsensitive) != 0) {
            row.flightRing = row.sandboxId;
        }
        row.eventCount = pickInt(obj, {"eventCount", "EventCount", "hitCount", "count"}, 0);
        row.cabId = pickString(obj, {"cabId", "CabId", "cabFileId"});
        row.stackTrace = pickString(obj, {"stackTrace", "StackTrace", "failureStack"});
        row.firstSeenAt = pickString(obj, {"firstSeen", "FirstSeen", "firstOccurrence"});
        row.lastSeenAt = pickString(obj, {"lastSeen", "LastSeen", "lastOccurrence"});
        row.applicationName = pickString(obj, {"applicationName", "ApplicationName"});
        if (!row.dateUtc.isEmpty()) {
            rows.push_back(std::move(row));
        }
    }
    return rows;
}

QVector<FailureHitRow> parseFailureHits(const QByteArray& json, const QString& storeId)
{
    return parseFailureHitObjects(parsePage(json).values, storeId);
}

QString normalizeMetricType(QString raw)
{
    // Dev Center sometimes returns "Acqusition" (typo).
    if (raw.compare(QLatin1String("Acqusition"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("Acquisition");
    }
    return raw;
}

QVector<FunnelMetricRow> parseFunnel(const QByteArray& json,
                                     const QString& storeId,
                                     const QString& windowStart,
                                     const QString& windowEnd)
{
    QVector<FunnelMetricRow> rows;
    for (const QJsonObject& obj : parsePage(json).values) {
        FunnelMetricRow row;
        row.storeId = storeId;
        row.windowStart = windowStart;
        row.windowEnd = windowEnd;
        row.metricType = normalizeMetricType(pickString(obj, {"MetricType", "metricType"}));
        row.userCount = pickInt(obj, {"UserCount", "userCount", "Count"}, 0);
        row.market = pickString(obj, {"market", "Market"});
        row.deviceType = pickString(obj, {"deviceType", "DeviceType"});
        if (!row.metricType.isEmpty()) {
            rows.push_back(std::move(row));
        }
    }
    return rows;
}

QVector<ChannelConversionRow> parseChannelObjects(const QVector<QJsonObject>& values,
                                                  const QString& storeId)
{
    QVector<ChannelConversionRow> rows;
    rows.reserve(values.size());
    for (const QJsonObject& obj : values) {
        ChannelConversionRow row;
        row.storeId = storeId;
        row.dateUtc = normalizeDate(pickString(obj, {"date", "Date"}));
        row.channelType = pickString(obj, {"channelType", "ChannelType"});
        row.market = pickString(obj, {"market", "Market"});
        row.deviceType = pickString(obj, {"deviceType", "DeviceType"});
        row.referrerUriDomain = pickString(obj, {"referrerUriDomain", "ReferrerUriDomain"});
        row.customCampaignId = pickString(obj, {"customCampaignId", "CustomCampaignId", "campaignId"});
        row.clickCount = pickInt(obj, {"clickCount", "ClickCount"}, 0);
        row.conversionCount = pickInt(obj, {"conversionCount", "ConversionCount"}, 0);
        row.applicationName = pickString(obj, {"applicationName", "ApplicationName"});
        // Aggregates without a date still count toward totals — pin to window end later if needed.
        if (row.dateUtc.isEmpty() && (row.clickCount > 0 || row.conversionCount > 0)) {
            row.dateUtc = QStringLiteral("total");
        }
        if (!row.dateUtc.isEmpty()) {
            rows.push_back(std::move(row));
        }
    }
    return rows;
}

bool pickBool(const QJsonObject& obj, std::initializer_list<const char*> keys, bool fallback = false)
{
    for (const char* key : keys) {
        const QJsonValue v = obj.value(QLatin1String(key));
        if (v.isBool()) {
            return v.toBool();
        }
        if (v.isString()) {
            const QString s = v.toString().trimmed().toLower();
            if (s == QLatin1String("true") || s == QLatin1String("1")) {
                return true;
            }
            if (s == QLatin1String("false") || s == QLatin1String("0")) {
                return false;
            }
        }
        if (v.isDouble()) {
            return v.toInt() != 0;
        }
    }
    return fallback;
}

QVector<FailureDetailRow> parseFailureDetailObjects(const QVector<QJsonObject>& values,
                                                    const QString& storeId)
{
    QVector<FailureDetailRow> rows;
    rows.reserve(values.size());
    for (const QJsonObject& obj : values) {
        FailureDetailRow row;
        row.storeId = storeId;
        row.failureHash = pickString(obj, {"failureHash", "FailureHash"});
        row.failureName = pickString(obj, {"failureName", "FailureName"});
        row.dateUtc = normalizeDate(pickString(obj, {"date", "Date"}));
        row.cabId = pickString(obj, {"cabId", "CabId"});
        row.cabExpiration = pickString(obj, {"cabExpirationTime", "CabExpirationTime"});
        row.cabDownloadable = pickBool(obj, {"cabDownloadable", "CabDownloadable"});
        row.market = pickString(obj, {"market", "Market"});
        row.deviceType = pickString(obj, {"deviceType", "DeviceType"});
        row.deviceModel = pickString(obj, {"deviceModel", "DeviceModel"});
        row.osVersion = pickString(obj, {"osVersion", "OsVersion"});
        row.osBuild = pickString(obj, {"osBuild", "OsBuild"});
        row.osRelease = pickString(obj, {"osRelease", "OsRelease"});
        row.osArchitecture = pickString(obj, {"osArchitecture", "OsArchitecture"});
        row.packageVersion = pickString(obj, {"packageVersion", "PackageVersion"});
        row.cpuManufacturer = pickString(obj, {"cpuManufacturer", "CpuManufacturer"});
        row.cpuFamily = pickString(obj, {"cpuFamilyName", "CpuFamilyName", "cpuFamily"});
        row.cpuName = pickString(obj, {"cpuName", "CpuName"});
        row.flightRing = pickString(obj, {"flightRing", "FlightRing"});
        row.sandboxId = pickString(obj, {"sandboxId", "SandboxId"});
        row.eventCount = pickInt(obj, {"eventCount", "EventCount", "hitCount", "count"}, 0);
        if (!row.failureHash.isEmpty() || !row.cabId.isEmpty()) {
            rows.push_back(std::move(row));
        }
    }
    return rows;
}

QString formatStackTrace(const QVector<QJsonObject>& frames)
{
    QStringList lines;
    for (const QJsonObject& obj : frames) {
        const QString level = pickString(obj, {"level", "Level"});
        const QString image = pickString(obj, {"image", "Image"});
        const QString function = pickString(obj, {"function", "Function"});
        const QString offset = pickString(obj, {"offset", "Offset"});
        lines << QStringLiteral("#%1 %2!%3 + %4").arg(level, image, function, offset);
    }
    return lines.join(QLatin1Char('\n'));
}

QVector<InstallRow> parseInstallObjects(const QVector<QJsonObject>& values, const QString& storeId)
{
    QVector<InstallRow> rows;
    rows.reserve(values.size());
    for (const QJsonObject& obj : values) {
        InstallRow row;
        row.storeId = storeId;
        row.dateUtc = normalizeDate(pickString(obj, {"date", "Date"}));
        row.market = pickString(obj, {"market", "Market"});
        row.deviceType = pickString(obj, {"deviceType", "DeviceType"});
        row.osVersion = pickString(obj, {"osVersion", "OsVersion"});
        row.packageVersion = pickString(obj, {"packageVersion", "PackageVersion"});
        row.successfulInstallCount =
            pickInt(obj, {"successfulInstallCount", "SuccessfulInstallCount", "installCount"}, 0);
        row.applicationName = pickString(obj, {"applicationName", "ApplicationName"});
        if (!row.dateUtc.isEmpty()) {
            rows.push_back(std::move(row));
        }
    }
    return rows;
}

QVector<UsageDailyRow> parseUsageObjects(const QVector<QJsonObject>& values, const QString& storeId)
{
    QVector<UsageDailyRow> rows;
    rows.reserve(values.size());
    for (const QJsonObject& obj : values) {
        UsageDailyRow row;
        row.storeId = storeId;
        row.dateUtc = normalizeDate(pickString(obj, {"date", "Date"}));
        row.market = pickString(obj, {"market", "Market"});
        row.deviceType = pickString(obj, {"deviceType", "DeviceType"});
        row.packageVersion = pickString(obj, {"packageVersion", "PackageVersion"});
        row.dailySessionCount = pickInt(obj, {"dailySessionCount", "DailySessionCount"}, 0);
        row.engagementMinutes = static_cast<double>(
            pickInt(obj, {"engagementDurationMinutes", "EngagementDurationMinutes"}, 0));
        if (obj.contains(QStringLiteral("engagementDurationMinutes"))
            && obj.value(QStringLiteral("engagementDurationMinutes")).isDouble()) {
            row.engagementMinutes =
                obj.value(QStringLiteral("engagementDurationMinutes")).toDouble();
        }
        row.dailyActiveUsers = pickInt(obj, {"dailyActiveUsers", "DailyActiveUsers"}, 0);
        row.dailyActiveDevices = pickInt(obj, {"dailyActiveDevices", "DailyActiveDevices"}, 0);
        row.dailyNewUsers = pickInt(obj, {"dailyNewUsers", "DailyNewUsers"}, 0);
        row.applicationName = pickString(obj, {"applicationName", "ApplicationName"});
        if (!row.dateUtc.isEmpty() || row.dailyActiveUsers > 0 || row.dailySessionCount > 0) {
            if (row.dateUtc.isEmpty()) {
                row.dateUtc = QStringLiteral("unknown");
            }
            rows.push_back(std::move(row));
        }
    }
    return rows;
}

QVector<RatingRow> parseRatingObjects(const QVector<QJsonObject>& values, const QString& storeId)
{
    QVector<RatingRow> rows;
    rows.reserve(values.size());
    for (const QJsonObject& obj : values) {
        RatingRow row;
        row.storeId = storeId;
        row.dateUtc = normalizeDate(pickString(obj, {"date", "Date"}));
        row.market = pickString(obj, {"market", "Market"});
        row.deviceType = pickString(obj, {"deviceType", "DeviceType"});
        row.osVersion = pickString(obj, {"osVersion", "OsVersion"});
        // API uses plural keys: oneStar / twoStars / threeStars / ...
        row.oneStar = pickInt(obj, {"oneStar", "OneStar", "oneStars"}, 0);
        row.twoStar = pickInt(obj, {"twoStars", "TwoStars", "twoStar"}, 0);
        row.threeStar = pickInt(obj, {"threeStars", "ThreeStars", "threeStar"}, 0);
        row.fourStar = pickInt(obj, {"fourStars", "FourStars", "fourStar"}, 0);
        row.fiveStar = pickInt(obj, {"fiveStars", "FiveStars", "fiveStar"}, 0);
        row.ratingCount = pickInt(obj, {"ratingCount", "RatingCount", "totalRatings"}, 0);
        if (row.ratingCount <= 0) {
            row.ratingCount = row.oneStar + row.twoStar + row.threeStar + row.fourStar + row.fiveStar;
        }
        if (obj.contains(QStringLiteral("averageRating"))) {
            row.averageRating = obj.value(QStringLiteral("averageRating")).toDouble();
        } else if (obj.contains(QStringLiteral("AverageRating"))) {
            row.averageRating = obj.value(QStringLiteral("AverageRating")).toDouble();
        } else if (row.ratingCount > 0) {
            const double weighted =
                (1.0 * row.oneStar) + (2.0 * row.twoStar) + (3.0 * row.threeStar)
                + (4.0 * row.fourStar) + (5.0 * row.fiveStar);
            row.averageRating = weighted / static_cast<double>(row.ratingCount);
        }
        if (!row.dateUtc.isEmpty() || row.ratingCount > 0) {
            if (row.dateUtc.isEmpty()) {
                row.dateUtc = QStringLiteral("unknown");
            }
            rows.push_back(std::move(row));
        }
    }
    return rows;
}

QVector<ReviewRow> parseReviewObjects(const QVector<QJsonObject>& values, const QString& storeId)
{
    QVector<ReviewRow> rows;
    rows.reserve(values.size());
    for (const QJsonObject& obj : values) {
        ReviewRow row;
        row.storeId = storeId;
        row.reviewId = pickString(obj, {"reviewId", "ReviewId", "id", "Id"});
        row.dateUtc = normalizeDate(pickString(obj, {"date", "Date", "reviewDate"}));
        row.rating = static_cast<int>(pickInt(obj, {"rating", "Rating", "averageRating"}, 0));
        row.title = pickString(obj, {"reviewTitle", "ReviewTitle", "title", "Title"});
        row.reviewText = pickString(obj, {"reviewText", "ReviewText", "text", "Text"});
        row.market = pickString(obj, {"market", "Market"});
        row.osVersion = pickString(obj, {"osVersion", "OsVersion"});
        row.deviceType = pickString(obj, {"deviceType", "DeviceType"});
        row.packageVersion = pickString(obj, {"packageVersion", "PackageVersion"});
        row.reviewerName = pickString(obj, {"reviewerName", "ReviewerName"});
        row.helpfulCount = pickInt(obj, {"helpfulCount", "HelpfulCount"}, 0);
        row.deviceModel = pickString(obj, {"deviceModel", "DeviceModel"});
        row.deviceRamMb = pickInt(obj, {"deviceRAM", "DeviceRAM", "deviceRam"}, 0);
        row.deviceStorageGb = pickDouble(obj, {"deviceStorageCapacity", "DeviceStorageCapacity"}, 0);
        row.deviceScreenResolution =
            pickString(obj, {"deviceScreenResolution", "DeviceScreenResolution"});
        row.productFamily = pickString(obj, {"productFamily", "ProductFamily"});
        row.isRevised = pickBool(obj, {"isRevised", "IsRevised"});
        row.isTouchEnabled = pickBool(obj, {"isTouchEnabled", "IsTouchEnabled"});
        // Topic tags: try common undocumented shapes (array or delimited string).
        if (obj.value(QStringLiteral("topicTags")).isArray()) {
            QStringList tags;
            for (const QJsonValue& v : obj.value(QStringLiteral("topicTags")).toArray()) {
                if (v.isString()) {
                    tags << v.toString();
                } else if (v.isObject()) {
                    const QString t = pickString(v.toObject(), {"tag", "name", "topic", "label"});
                    if (!t.isEmpty()) {
                        tags << t;
                    }
                }
            }
            row.topicTags = tags.join(QLatin1Char(';'));
        } else {
            row.topicTags = pickString(obj, {"topicTags", "topics", "reviewTopics", "tags"});
        }
        if (row.reviewId.isEmpty()) {
            row.reviewId = QStringLiteral("%1|%2|%3")
                               .arg(row.dateUtc, row.reviewerName, QString::number(row.rating));
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

} // namespace AnalyticsParsers
} // namespace storepulse
