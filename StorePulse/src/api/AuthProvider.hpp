#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <optional>

namespace storepulse {

struct AccessToken {
    QString value;
    QDateTime expiresAtUtc;
};

enum class AuthMode {
    InteractiveMicrosoft, // Hotmail / MSA + work/school via browser or device code
    ClientCredentials     // Entra app-only (advanced / headless)
};

enum class AccountAudience {
    Common,        // personal Microsoft accounts + work/school
    Consumers,     // personal only (Hotmail, Outlook.com, Live)
    Organizations  // work/school only
};

[[nodiscard]] inline QString audienceTenantPath(AccountAudience audience)
{
    switch (audience) {
    case AccountAudience::Consumers: return QStringLiteral("consumers");
    case AccountAudience::Organizations: return QStringLiteral("organizations");
    case AccountAudience::Common:
    default: return QStringLiteral("common");
    }
}

/// Common token source for Partner Center / Dev Center API calls.
class AuthProvider : public QObject {
    Q_OBJECT
public:
    static constexpr auto kDevCenterResource = "https://manage.devcenter.microsoft.com";
    static constexpr int kRefreshSkewSeconds = 120;

    explicit AuthProvider(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    [[nodiscard]] virtual AuthMode mode() const = 0;
    [[nodiscard]] virtual bool hasCachedToken() const = 0;
    [[nodiscard]] virtual std::optional<AccessToken> cachedToken() const = 0;
    virtual void clearCachedToken() = 0;
    virtual void requestAccessToken() = 0;
    [[nodiscard]] virtual QString lastError() const = 0;

signals:
    void tokenReady(const QString& accessToken, const QDateTime& expiresAtUtc);
    void tokenFailed(const QString& error);
    void interactiveSignInRequired(const QString& message);
    /// Device-code UX: show userCode and verification URL.
    void deviceCodeReceived(const QString& userCode, const QString& verificationUrl);
};

} // namespace storepulse
