#include "InsightsPageviewsClient.hpp"

#include "Logger.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrlQuery>

namespace storepulse {

InsightsPageviewsClient::InsightsPageviewsClient(QNetworkAccessManager* nam, QObject* parent)
    : QObject(parent)
    , m_nam(nam)
{
}

QString InsightsPageviewsClient::cookieFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/partner_cookies.txt");
}

bool InsightsPageviewsClient::hasCookieFile()
{
    QFile f(cookieFilePath());
    return f.exists() && f.size() > 0;
}

void InsightsPageviewsClient::fetch(const QString& storeId, const QDate& fromUtc, const QDate& toUtc)
{
    QFile cookieFile(cookieFilePath());
    if (!cookieFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit finished(storeId, false,
                      QStringLiteral("Missing Partner Center cookie file: %1").arg(cookieFilePath()),
                      {});
        return;
    }
    const QByteArray cookie = cookieFile.readAll().trimmed();
    if (cookie.isEmpty()) {
        emit finished(storeId, false, QStringLiteral("Partner Center cookie file is empty"), {});
        return;
    }

    QUrl url(QString::fromUtf8(kEndpoint));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("applicationId"), storeId);
    q.addQueryItem(QStringLiteral("startDate"), fromUtc.toString(Qt::ISODate));
    q.addQueryItem(QStringLiteral("endDate"), toUtc.toString(Qt::ISODate));
    q.addQueryItem(QStringLiteral("aggregationLevel"), QStringLiteral("day"));
    q.addQueryItem(QStringLiteral("groupby"), QStringLiteral("date"));
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("Cookie", cookie);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("StoreVitals/1.0"));

    Logger::info(QStringLiteral("Insights pageviews GET %1").arg(url.toString()));
    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, storeId]() {
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300) {
            emit finished(storeId, false,
                          QStringLiteral("Insights pageviews HTTP %1: %2")
                              .arg(status)
                              .arg(QString::fromUtf8(body.left(240))),
                          {});
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(body);
        const QJsonArray values = doc.object().value(QStringLiteral("Value")).toArray();
        QVector<InsightsPageviewRow> rows;
        rows.reserve(values.size());
        for (const QJsonValue& v : values) {
            const QJsonObject o = v.toObject();
            InsightsPageviewRow row;
            row.storeId = storeId;
            const QString dateRaw = o.value(QStringLiteral("date")).toString();
            row.dateUtc = dateRaw.left(10);
            row.pageviewsCount = static_cast<qint64>(
                o.value(QStringLiteral("pageviewsCount")).toDouble());
            if (!row.dateUtc.isEmpty()) {
                rows.push_back(row);
            }
        }
        emit finished(storeId, true, {}, rows);
    });
}

} // namespace storepulse
