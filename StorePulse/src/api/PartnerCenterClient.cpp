#include "PartnerCenterClient.hpp"

#include "AnalyticsParsers.hpp"
#include "Logger.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

#include <algorithm>

namespace storepulse {

PartnerCenterClient::PartnerCenterClient(AuthProvider& auth,
                                         QNetworkAccessManager* nam,
                                         QObject* parent)
    : QObject(parent)
    , m_auth(auth)
    , m_nam(nam)
{
    m_paceTimer.setSingleShot(true);
    connect(&m_paceTimer, &QTimer::timeout, this, &PartnerCenterClient::continueAuthorizedFetch);
    ensureAuthHooks();
}

void PartnerCenterClient::ensureAuthHooks()
{
    if (m_authHooksConnected) {
        return;
    }
    connect(&m_auth, &AuthProvider::tokenReady, this, &PartnerCenterClient::onTokenReady);
    connect(&m_auth, &AuthProvider::tokenFailed, this, &PartnerCenterClient::onTokenFailed);
    m_authHooksConnected = true;
}

void PartnerCenterClient::setApiBaseOverride(const QUrl& base) { m_baseOverride = base; }
RateLimiter& PartnerCenterClient::rateLimiter() { return m_rateLimiter; }
QString PartnerCenterClient::lastError() const { return m_lastError; }

QString PartnerCenterClient::endpointPath(Endpoint endpoint) const
{
    switch (endpoint) {
    case Endpoint::AppAcquisitions:
    case Endpoint::AppRevenue: return QStringLiteral("appacquisitions");
    case Endpoint::InAppAcquisitions: return QStringLiteral("inappacquisitions");
    case Endpoint::Funnel: return QStringLiteral("funnel");
    case Endpoint::ChannelConversions: return QStringLiteral("appchannelconversions");
    case Endpoint::Installs: return QStringLiteral("installs");
    case Endpoint::UsageDaily: return QStringLiteral("usagedaily");
    case Endpoint::Ratings: return QStringLiteral("ratings");
    case Endpoint::Reviews: return QStringLiteral("reviews");
    case Endpoint::FailureHits: return QStringLiteral("failurehits");
    case Endpoint::FailureDetails: return QStringLiteral("failuredetails");
    case Endpoint::StackTrace: return QStringLiteral("stacktrace");
    case Endpoint::CabDownload: return QStringLiteral("cabdownload");
    }
    return {};
}

QUrl PartnerCenterClient::buildUrl(Endpoint endpoint, const AnalyticsQuery& query) const
{
    QUrl base = m_baseOverride.isValid() ? m_baseOverride
                                         : QUrl(QLatin1String(kAnalyticsBase));

    QString path = base.path();
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }
    path += endpointPath(endpoint);
    base.setPath(path);

    QUrlQuery q;
    q.addQueryItem(QStringLiteral("applicationId"), query.applicationId);
    if (query.startDate.isValid()
        && endpoint != Endpoint::StackTrace
        && endpoint != Endpoint::CabDownload) {
        q.addQueryItem(QStringLiteral("startDate"), query.startDate.toString(Qt::ISODate));
    }
    if (query.endDate.isValid()
        && endpoint != Endpoint::StackTrace
        && endpoint != Endpoint::CabDownload) {
        q.addQueryItem(QStringLiteral("endDate"), query.endDate.toString(Qt::ISODate));
    }
    if (endpoint != Endpoint::Funnel
        && endpoint != Endpoint::StackTrace
        && endpoint != Endpoint::CabDownload) {
        q.addQueryItem(QStringLiteral("top"), QString::number(query.top));
        if (query.skip > 0) {
            q.addQueryItem(QStringLiteral("skip"), QString::number(query.skip));
        }
    }
    if (!query.groupBy.trimmed().isEmpty()) {
        q.addQueryItem(QStringLiteral("groupby"), query.groupBy.trimmed());
    }
    if (!query.orderBy.trimmed().isEmpty()) {
        q.addQueryItem(QStringLiteral("orderby"), query.orderBy.trimmed());
    }
    if (!query.filter.trimmed().isEmpty()) {
        q.addQueryItem(QStringLiteral("filter"), query.filter.trimmed());
    }
    if (!query.aggregationLevel.trimmed().isEmpty()) {
        q.addQueryItem(QStringLiteral("aggregationLevel"), query.aggregationLevel.trimmed());
    }
    if (!query.failureHash.trimmed().isEmpty()) {
        q.addQueryItem(QStringLiteral("failureHash"), query.failureHash.trimmed());
    }
    if (!query.cabId.trimmed().isEmpty()) {
        q.addQueryItem(QStringLiteral("cabId"), query.cabId.trimmed());
    }
    base.setQuery(q);
    return base;
}

void PartnerCenterClient::fetchAppAcquisitions(const AnalyticsQuery& query)
{
    startAuthorizedFetch(PendingFetch{Endpoint::AppAcquisitions, query, {}, true, {}});
}
void PartnerCenterClient::fetchAppRevenue(const AnalyticsQuery& query)
{
    startAuthorizedFetch(PendingFetch{Endpoint::AppRevenue, query, {}, true, {}});
}
void PartnerCenterClient::fetchInAppAcquisitions(const AnalyticsQuery& query)
{
    startAuthorizedFetch(PendingFetch{Endpoint::InAppAcquisitions, query, {}, true, {}});
}
void PartnerCenterClient::fetchFunnel(const AnalyticsQuery& query)
{
    startAuthorizedFetch(PendingFetch{Endpoint::Funnel, query, {}, false, {}});
}
void PartnerCenterClient::fetchChannelConversions(const AnalyticsQuery& query)
{
    startAuthorizedFetch(PendingFetch{Endpoint::ChannelConversions, query, {}, true, {}});
}
void PartnerCenterClient::fetchInstalls(const AnalyticsQuery& query)
{
    startAuthorizedFetch(PendingFetch{Endpoint::Installs, query, {}, true, {}});
}
void PartnerCenterClient::fetchUsageDaily(const AnalyticsQuery& query)
{
    startAuthorizedFetch(PendingFetch{Endpoint::UsageDaily, query, {}, true, {}});
}
void PartnerCenterClient::fetchRatings(const AnalyticsQuery& query)
{
    startAuthorizedFetch(PendingFetch{Endpoint::Ratings, query, {}, true, {}});
}
void PartnerCenterClient::fetchReviews(const AnalyticsQuery& query)
{
    startAuthorizedFetch(PendingFetch{Endpoint::Reviews, query, {}, true, {}});
}
void PartnerCenterClient::fetchFailureHits(const AnalyticsQuery& query)
{
    startAuthorizedFetch(PendingFetch{Endpoint::FailureHits, query, {}, true, {}});
}
void PartnerCenterClient::fetchFailureDetails(const AnalyticsQuery& query)
{
    startAuthorizedFetch(PendingFetch{Endpoint::FailureDetails, query, {}, true, {}});
}
void PartnerCenterClient::fetchStackTrace(const AnalyticsQuery& query)
{
    startAuthorizedFetch(PendingFetch{Endpoint::StackTrace, query, {}, false, {}});
}
void PartnerCenterClient::downloadCab(const QString& storeId, const QString& cabId, const QString& destPath)
{
    AnalyticsQuery q;
    q.applicationId = storeId;
    q.cabId = cabId;
    startAuthorizedFetch(PendingFetch{Endpoint::CabDownload, q, {}, false, destPath});
}

void PartnerCenterClient::startAuthorizedFetch(PendingFetch pending)
{
    m_lastError.clear();
    m_paceTimer.stop();
    m_pending = std::move(pending);
    continueAuthorizedFetch();
}

void PartnerCenterClient::continueAuthorizedFetch()
{
    if (!m_pending) {
        return;
    }
    const int wait = m_rateLimiter.secondsUntilNextAllowed();
    if (wait > 0) {
        Logger::info(QStringLiteral("Rate limit pacing: waiting %1s before %2")
                         .arg(wait)
                         .arg(endpointPath(m_pending->endpoint)));
        m_paceTimer.start(wait * 1000);
        return;
    }
    m_auth.requestAccessToken();
}

void PartnerCenterClient::emitFinished(Endpoint endpoint,
                                       const QString& storeId,
                                       bool ok,
                                       const QString& error)
{
    switch (endpoint) {
    case Endpoint::AppAcquisitions: emit acquisitionsFinished(storeId, ok, error); break;
    case Endpoint::AppRevenue:
    case Endpoint::InAppAcquisitions: emit revenueFinished(storeId, ok, error); break;
    case Endpoint::Funnel: emit funnelFinished(storeId, ok, error, {}); break;
    case Endpoint::ChannelConversions: emit channelFinished(storeId, ok, error); break;
    case Endpoint::Installs: emit installsFinished(storeId, ok, error); break;
    case Endpoint::UsageDaily: emit usageFinished(storeId, ok, error); break;
    case Endpoint::Ratings: emit ratingsFinished(storeId, ok, error); break;
    case Endpoint::Reviews: emit reviewsFinished(storeId, ok, error); break;
    case Endpoint::FailureHits: emit failureHitsFinished(storeId, ok, error); break;
    case Endpoint::FailureDetails: emit failureDetailsFinished(storeId, ok, error); break;
    case Endpoint::StackTrace:
        emit stackTraceFinished(storeId, m_pending ? m_pending->query.cabId : QString(), ok, {}, error);
        break;
    case Endpoint::CabDownload:
        emit cabDownloadFinished(storeId, m_pending ? m_pending->query.cabId : QString(), ok, {}, error);
        break;
    }
}

void PartnerCenterClient::onTokenFailed(const QString& error)
{
    if (!m_pending) {
        return;
    }
    const QString storeId = m_pending->query.applicationId;
    const Endpoint endpoint = m_pending->endpoint;
    const QString cabId = m_pending->query.cabId;
    m_pending.reset();
    m_lastError = error;
    if (endpoint == Endpoint::StackTrace) {
        emit stackTraceFinished(storeId, cabId, false, {}, error);
    } else if (endpoint == Endpoint::CabDownload) {
        emit cabDownloadFinished(storeId, cabId, false, {}, error);
    } else {
        emitFinished(endpoint, storeId, false, error);
    }
}

void PartnerCenterClient::onTokenReady(const QString& token, const QDateTime&)
{
    if (!m_pending) {
        return;
    }

    const PendingFetch job = *m_pending;
    QUrl url = job.nextUrl.isEmpty() ? buildUrl(job.endpoint, job.query) : QUrl(job.nextUrl);
    if (!job.nextUrl.isEmpty() && url.isRelative()) {
        const QUrl base = m_baseOverride.isValid() ? m_baseOverride
                                                   : QUrl(QLatin1String(kAnalyticsBase));
        url = base.resolved(url);
    }

    QNetworkRequest req(url);
    req.setRawHeader("Authorization", QByteArray("Bearer ") + token.toUtf8());
    if (job.endpoint == Endpoint::CabDownload) {
        req.setRawHeader("Accept", "application/octet-stream");
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    } else {
        req.setRawHeader("Accept", "application/json");
    }

    Logger::info(QStringLiteral("GET %1").arg(url.toString(QUrl::RemoveUserInfo)));
    m_rateLimiter.noteRequestSent();
    QNetworkReply* reply = m_nam->get(req);
    if (job.endpoint == Endpoint::CabDownload) {
        connect(reply, &QNetworkReply::finished, this, [this, reply, job]() {
            onCabReply(reply, job.query.applicationId, job.query.cabId, job.destPath);
        });
    } else {
        connect(reply, &QNetworkReply::finished, this, [this, reply, job]() {
            onAnalyticsReply(reply, job.endpoint, job.query.applicationId, job.followPagination);
        });
    }
}

void PartnerCenterClient::onCabReply(QNetworkReply* reply,
                                     QString storeId,
                                     QString cabId,
                                     QString destPath)
{
    reply->deleteLater();
    m_rateLimiter.observeReply(reply);
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    m_pending.reset();

    if (reply->error() != QNetworkReply::NoError || status >= 400 || body.isEmpty()) {
        m_lastError = QStringLiteral("CAB download failed (%1): %2")
                          .arg(status)
                          .arg(reply->errorString());
        Logger::error(m_lastError);
        emit cabDownloadFinished(storeId, cabId, false, {}, m_lastError);
        return;
    }

    QFileInfo info(destPath);
    QDir().mkpath(info.absolutePath());
    QFile file(destPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastError = QStringLiteral("Could not write CAB to %1").arg(destPath);
        emit cabDownloadFinished(storeId, cabId, false, {}, m_lastError);
        return;
    }
    file.write(body);
    file.close();
    Logger::info(QStringLiteral("CAB saved %1 (%2 bytes)").arg(destPath).arg(body.size()));
    emit cabDownloadFinished(storeId, cabId, true, destPath, {});
}

void PartnerCenterClient::onAnalyticsReply(QNetworkReply* reply,
                                           Endpoint endpoint,
                                           QString storeId,
                                           bool follow)
{
    reply->deleteLater();
    m_rateLimiter.observeReply(reply);

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    const QString cabId = m_pending ? m_pending->query.cabId : QString();

    if (reply->error() != QNetworkReply::NoError || status >= 400) {
        m_lastError = QStringLiteral("Analytics request failed (%1): %2")
                          .arg(status)
                          .arg(reply->errorString());
        if (status == 401 || status == 403) {
            m_lastError += QStringLiteral(
                " - Token was issued, but Partner Center has not authorized this Entra app "
                "for analytics yet.");
        } else if (status == 429) {
            m_lastError += QStringLiteral(" - rate limited; wait ~%1s then sync again.")
                               .arg(std::max(1, m_rateLimiter.secondsRemaining()));
        }
        Logger::error(m_lastError);
        m_pending.reset();
        if (endpoint == Endpoint::StackTrace) {
            emit stackTraceFinished(storeId, cabId, false, {}, m_lastError);
        } else {
            emitFinished(endpoint, storeId, false, m_lastError);
        }
        return;
    }

    if (endpoint == Endpoint::StackTrace) {
        const auto page = AnalyticsParsers::parsePage(body);
        const QString stack = AnalyticsParsers::formatStackTrace(page.values);
        m_pending.reset();
        emit stackTraceFinished(storeId, cabId, true, stack, {});
        return;
    }

    const auto page = AnalyticsParsers::parsePage(body);
    QString next;

    switch (endpoint) {
    case Endpoint::AppAcquisitions: {
        const auto rows = AnalyticsParsers::parseAcquisitionObjects(page.values, storeId);
        emit acquisitionsPage(storeId, rows, page.nextLink);
        next = page.nextLink;
        break;
    }
    case Endpoint::AppRevenue: {
        emit revenuePage(storeId,
                         AnalyticsParsers::parseRevenueObjects(page.values, storeId, QStringLiteral("app")),
                         page.nextLink);
        next = page.nextLink;
        break;
    }
    case Endpoint::InAppAcquisitions: {
        emit revenuePage(storeId,
                         AnalyticsParsers::parseRevenueObjects(page.values, storeId, QStringLiteral("iap")),
                         page.nextLink);
        next = page.nextLink;
        break;
    }
    case Endpoint::Funnel: {
        QString start;
        QString end;
        if (m_pending) {
            start = m_pending->query.startDate.toString(Qt::ISODate);
            end = m_pending->query.endDate.toString(Qt::ISODate);
        }
        const auto rows = AnalyticsParsers::parseFunnel(body, storeId, start, end);
        m_pending.reset();
        emit funnelFinished(storeId, true, {}, rows);
        return;
    }
    case Endpoint::ChannelConversions: {
        emit channelPage(storeId, AnalyticsParsers::parseChannelObjects(page.values, storeId), page.nextLink);
        next = page.nextLink;
        break;
    }
    case Endpoint::Installs: {
        emit installsPage(storeId, AnalyticsParsers::parseInstallObjects(page.values, storeId), page.nextLink);
        next = page.nextLink;
        break;
    }
    case Endpoint::UsageDaily: {
        emit usagePage(storeId, AnalyticsParsers::parseUsageObjects(page.values, storeId), page.nextLink);
        next = page.nextLink;
        break;
    }
    case Endpoint::Ratings: {
        emit ratingsPage(storeId, AnalyticsParsers::parseRatingObjects(page.values, storeId), page.nextLink);
        next = page.nextLink;
        break;
    }
    case Endpoint::Reviews: {
        emit reviewsPage(storeId, AnalyticsParsers::parseReviewObjects(page.values, storeId), page.nextLink);
        next = page.nextLink;
        break;
    }
    case Endpoint::FailureHits: {
        emit failureHitsPage(storeId, AnalyticsParsers::parseFailureHitObjects(page.values, storeId), page.nextLink);
        next = page.nextLink;
        break;
    }
    case Endpoint::FailureDetails: {
        emit failureDetailsPage(storeId,
                                AnalyticsParsers::parseFailureDetailObjects(page.values, storeId),
                                page.nextLink);
        next = page.nextLink;
        break;
    }
    case Endpoint::StackTrace:
    case Endpoint::CabDownload:
        break;
    }

    if (follow && !next.isEmpty() && m_pending) {
        m_pending->nextUrl = next;
        continueAuthorizedFetch();
        return;
    }

    m_pending.reset();
    emitFinished(endpoint, storeId, true, {});
}

} // namespace storepulse
