#pragma once

#include "Models.hpp"

#include <QDate>
#include <QNetworkAccessManager>
#include <QObject>
#include <QVector>

namespace storepulse {

/// Pulls Partner Center Insights overview page views
/// (GET /storepartneranalytics/api/v1/Acquisitions/pageviews).
/// Auth is cookie-based (Partner Center web session), not Dev Center client credentials.
class InsightsPageviewsClient : public QObject {
    Q_OBJECT
public:
    static constexpr auto kEndpoint =
        "https://partner.microsoft.com/storepartneranalytics/api/v1/Acquisitions/pageviews";

    explicit InsightsPageviewsClient(QNetworkAccessManager* nam, QObject* parent = nullptr);

    /// Cookie header file: %APPDATA%/StoreVitals/partner_cookies.txt
    [[nodiscard]] static QString cookieFilePath();
    [[nodiscard]] static bool hasCookieFile();

    void fetch(const QString& storeId, const QDate& fromUtc, const QDate& toUtc);

signals:
    void finished(const QString& storeId, bool ok, const QString& error,
                  const QVector<InsightsPageviewRow>& rows);

private:
    QNetworkAccessManager* m_nam = nullptr;
};

} // namespace storepulse
