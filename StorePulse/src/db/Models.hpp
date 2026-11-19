#pragma once

#include <QString>
#include <cstdint>

namespace storepulse {

struct AcquisitionRow {
    QString storeId;
    QString dateUtc;
    QString granularity;
    QString acquisitionType;
    QString market;
    QString deviceType;
    QString osVersion;
    QString ageGroup;
    QString gender;
    QString storeClient;
    QString channel;
    QString campaignId;
    qint64 quantity = 0;
    double purchasePriceUsd = 0;
    double purchaseTaxUsd = 0;
    QString source;
    QString applicationName;
};

/// Gross sales row (app package or IAP) from Partner Center analytics.
struct RevenueRow {
    QString storeId;
    QString dateUtc;
    QString source; // app | iap
    QString productId;
    QString productName;
    QString acquisitionType;
    QString market;
    qint64 quantity = 0;
    double purchasePriceUsd = 0;
    double purchaseTaxUsd = 0;
};

struct FailureHitRow {
    QString storeId;
    QString dateUtc;
    QString failureName;
    QString failureHash;
    QString failureStackHash;
    QString osVersion;
    QString osRelease;
    QString osArchitecture;
    QString packageName;
    QString packageVersion;
    QString eventType;
    QString market;
    QString deviceType;
    QString cpuManufacturer;
    QString cpuFamily;
    QString cpuName;
    QString sandboxId;
    QString flightRing;
    qint64 eventCount = 0;
    QString cabId;
    QString stackTrace;
    QString firstSeenAt;
    QString lastSeenAt;
    QString applicationName;
};

/// Per-CAB / device rows from analytics/failuredetails (last 30 days).
struct FailureDetailRow {
    QString storeId;
    QString failureHash;
    QString failureName;
    QString dateUtc;
    QString cabId;
    QString cabExpiration;
    bool cabDownloadable = false;
    QString market;
    QString deviceType;
    QString deviceModel;
    QString osVersion;
    QString osBuild;
    QString osRelease;
    QString osArchitecture;
    QString packageVersion;
    QString cpuManufacturer;
    QString cpuFamily;
    QString cpuName;
    QString flightRing;
    QString sandboxId;
    qint64 eventCount = 0;
    QString stackTrace;
};

struct FunnelMetricRow {
    QString storeId;
    QString windowStart;
    QString windowEnd;
    QString metricType;
    qint64 userCount = 0;
    QString market;
    QString deviceType;
};

struct ChannelConversionRow {
    QString storeId;
    QString dateUtc;
    QString channelType;
    QString market;
    QString deviceType;
    QString referrerUriDomain;
    QString customCampaignId;
    qint64 clickCount = 0;
    qint64 conversionCount = 0;
    QString applicationName;
};

/// Partner Center Insights AcqV2 overview page views (storepartneranalytics).
struct InsightsPageviewRow {
    QString storeId;
    QString dateUtc;
    qint64 pageviewsCount = 0;
};

struct InstallRow {
    QString storeId;
    QString dateUtc;
    QString market;
    QString deviceType;
    QString osVersion;
    QString packageVersion;
    qint64 successfulInstallCount = 0;
    QString applicationName;
};

struct UsageDailyRow {
    QString storeId;
    QString dateUtc;
    QString market;
    QString deviceType;
    QString packageVersion;
    qint64 dailySessionCount = 0;
    double engagementMinutes = 0;
    qint64 dailyActiveUsers = 0;
    qint64 dailyActiveDevices = 0;
    qint64 dailyNewUsers = 0;
    QString applicationName;
};

struct RatingRow {
    QString storeId;
    QString dateUtc;
    QString market;
    QString deviceType;
    QString osVersion;
    double averageRating = 0;
    qint64 ratingCount = 0;
    qint64 oneStar = 0;
    qint64 twoStar = 0;
    qint64 threeStar = 0;
    qint64 fourStar = 0;
    qint64 fiveStar = 0;
};

struct ReviewRow {
    QString storeId;
    QString reviewId;
    QString dateUtc;
    int rating = 0;
    QString title;
    QString reviewText;
    QString market;
    QString osVersion;
    QString deviceType;
    QString packageVersion;
    QString reviewerName;
    qint64 helpfulCount = 0;
    QString deviceModel;
    qint64 deviceRamMb = 0;
    double deviceStorageGb = 0;
    QString deviceScreenResolution;
    QString productFamily;
    bool isRevised = false;
    bool isTouchEnabled = false;
    QString topicTags; // JSON array or semicolon-separated tags
};

struct AppRow {
    QString storeId;
    QString displayName;
    QString packageFamily;
    bool isActive = true;
};

} // namespace storepulse
