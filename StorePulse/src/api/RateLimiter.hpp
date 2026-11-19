#pragma once

#include <QDateTime>
#include <QNetworkReply>

namespace storepulse {

/// Proactive spacing + reactive HTTP 429 / 503 backoff for Partner Center calls.
class RateLimiter {
public:
    /// Minimum gap between outbound analytics requests (probe/sync pacing).
    static constexpr int kDefaultMinIntervalSeconds = 4;
    /// Floor when Retry-After is missing or too small on 429.
    static constexpr int kDefault429FloorSeconds = 30;

    void setMinIntervalSeconds(int seconds);
    [[nodiscard]] int minIntervalSeconds() const;

    /// Call immediately before each outbound GET.
    void noteRequestSent();

    /// Parse Retry-After (seconds or HTTP-date) and arm backoff.
    void observeReply(QNetworkReply* reply);

    void forceBackoff(int seconds);

    [[nodiscard]] bool isBackingOff() const;
    [[nodiscard]] int secondsRemaining() const;
    [[nodiscard]] QDateTime backoffUntilUtc() const;

    /// Seconds to wait before the next request (backoff + min interval).
    [[nodiscard]] int secondsUntilNextAllowed() const;

    /// Exponential backoff helper when Retry-After is absent (base 2^attempt, capped).
    static int exponentialBackoffSeconds(int attempt, int baseSeconds = 2, int maxSeconds = 300);

private:
    QDateTime m_backoffUntilUtc;
    QDateTime m_lastRequestUtc;
    int m_minIntervalSeconds = kDefaultMinIntervalSeconds;
    int m_consecutiveThrottle = 0;
};

} // namespace storepulse
