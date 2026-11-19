#include "RateLimiter.hpp"

#include "Logger.hpp"

#include <QDateTime>
#include <algorithm>

namespace storepulse {

void RateLimiter::setMinIntervalSeconds(int seconds)
{
    m_minIntervalSeconds = std::max(0, seconds);
}

int RateLimiter::minIntervalSeconds() const
{
    return m_minIntervalSeconds;
}

void RateLimiter::noteRequestSent()
{
    m_lastRequestUtc = QDateTime::currentDateTimeUtc();
}

void RateLimiter::observeReply(QNetworkReply* reply)
{
    if (!reply) {
        return;
    }

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status != 429 && status != 503) {
        m_consecutiveThrottle = 0;
        return;
    }

    ++m_consecutiveThrottle;

    int waitSeconds = 0;
    const QByteArray retryAfter = reply->rawHeader("Retry-After");
    if (!retryAfter.isEmpty()) {
        bool ok = false;
        waitSeconds = retryAfter.toInt(&ok);
        if (!ok) {
            const QDateTime when = QDateTime::fromString(QString::fromUtf8(retryAfter),
                                                         Qt::RFC2822Date);
            if (when.isValid()) {
                waitSeconds = static_cast<int>(
                    QDateTime::currentDateTimeUtc().secsTo(when.toUTC()));
            }
        }
    }

    if (waitSeconds <= 0) {
        waitSeconds = exponentialBackoffSeconds(m_consecutiveThrottle,
                                                kDefault429FloorSeconds / 2,
                                                300);
    }

    // Dev Center often needs a longer cool-down than a 1–2s Retry-After.
    waitSeconds = std::max(waitSeconds, kDefault429FloorSeconds);
    waitSeconds = std::max(waitSeconds,
                           exponentialBackoffSeconds(m_consecutiveThrottle - 1,
                                                     kDefault429FloorSeconds,
                                                     300));

    forceBackoff(waitSeconds);
    Logger::warning(QStringLiteral("HTTP %1 — backing off %2s (throttle #%3)")
                        .arg(status)
                        .arg(waitSeconds)
                        .arg(m_consecutiveThrottle));
}

void RateLimiter::forceBackoff(int seconds)
{
    m_backoffUntilUtc = QDateTime::currentDateTimeUtc().addSecs(std::max(1, seconds));
}

bool RateLimiter::isBackingOff() const
{
    return m_backoffUntilUtc.isValid()
           && QDateTime::currentDateTimeUtc() < m_backoffUntilUtc;
}

int RateLimiter::secondsRemaining() const
{
    if (!isBackingOff()) {
        return 0;
    }
    return static_cast<int>(QDateTime::currentDateTimeUtc().secsTo(m_backoffUntilUtc));
}

QDateTime RateLimiter::backoffUntilUtc() const
{
    return m_backoffUntilUtc;
}

int RateLimiter::secondsUntilNextAllowed() const
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    int wait = 0;

    if (m_backoffUntilUtc.isValid() && now < m_backoffUntilUtc) {
        wait = static_cast<int>(now.secsTo(m_backoffUntilUtc));
    }

    if (m_lastRequestUtc.isValid() && m_minIntervalSeconds > 0) {
        const QDateTime earliest = m_lastRequestUtc.addSecs(m_minIntervalSeconds);
        if (now < earliest) {
            wait = std::max(wait, static_cast<int>(now.secsTo(earliest)));
        }
    }

    return std::max(0, wait);
}

int RateLimiter::exponentialBackoffSeconds(int attempt, int baseSeconds, int maxSeconds)
{
    const int clampedAttempt = std::max(0, std::min(attempt, 16));
    const qint64 value = static_cast<qint64>(baseSeconds) * (1LL << clampedAttempt);
    return static_cast<int>(std::min<qint64>(value, maxSeconds));
}

} // namespace storepulse
