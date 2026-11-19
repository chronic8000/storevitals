#pragma once

#include "AuthProvider.hpp"

#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QUrl>
#include <functional>
#include <optional>

namespace storepulse {

/// Sign in with Microsoft for Hotmail/MSA and work/school accounts.
/// Uses OAuth2 authorization code + PKCE (public client) by default,
/// with device-code fallback. Refresh tokens live in Credential Manager only.
class InteractiveAuthClient : public AuthProvider {
    Q_OBJECT
public:
    enum class Flow { AuthorizationCodePkce, DeviceCode };

    /// Sign-in scopes that work for Hotmail/MSA + work accounts.
    /// Dev Center analytics is NOT a published MSA delegated scope — use these for
    /// interactive identity; API calls use a separate Dev Center token when available.
    static constexpr auto kDefaultScope = "openid profile offline_access";
    static constexpr auto kDevCenterScope =
        "https://manage.devcenter.microsoft.com/.default offline_access";

    explicit InteractiveAuthClient(QNetworkAccessManager* nam, QObject* parent = nullptr);

    [[nodiscard]] AuthMode mode() const override { return AuthMode::InteractiveMicrosoft; }

    void setClientId(const QString& clientId);
    void setAudience(AccountAudience audience);
    void setScope(const QString& scope);
    void setPreferredFlow(Flow flow);
    void setAuthorityOverride(const QUrl& authorizeBase); // tests
    void setTokenEndpointOverride(const QUrl& tokenUrl);  // tests

    /// Credential Manager target for the refresh token (never logged).
    void setRefreshTokenTarget(const QString& targetName);

    [[nodiscard]] bool hasCachedToken() const override;
    [[nodiscard]] std::optional<AccessToken> cachedToken() const override;
    void clearCachedToken() override;
    void requestAccessToken() override;
    [[nodiscard]] QString lastError() const override;

    /// Force an interactive prompt even if a refresh token exists.
    void signInInteractively();
    void signOut();

    [[nodiscard]] bool hasRefreshToken() const;
    [[nodiscard]] QString clientId() const;
    [[nodiscard]] AccountAudience audience() const;

private:
    QNetworkAccessManager* m_nam = nullptr;
    QString m_clientId;
    AccountAudience m_audience = AccountAudience::Common;
    QString m_scope = QString::fromUtf8(kDefaultScope);
    Flow m_preferredFlow = Flow::AuthorizationCodePkce;
    QUrl m_authorityOverride;
    QUrl m_tokenOverride;
    QString m_refreshTarget = QStringLiteral("StoreVitals/MicrosoftRefreshToken");

    std::optional<AccessToken> m_cached;
    QString m_lastError;
    bool m_inFlight = false;

    QString m_pkceVerifier;
    QString m_oauthState;
    QUrl m_redirectUri;
    QTcpServer m_redirectServer;

    QString m_deviceCode;
    QString m_deviceInterval;
    int m_deviceExpiresIn = 0;

    [[nodiscard]] QUrl authorityRoot() const;
    [[nodiscard]] QUrl authorizeUrl() const;
    [[nodiscard]] QUrl tokenUrl() const;
    [[nodiscard]] bool tokenStillValid(int skewSeconds = kRefreshSkewSeconds) const;

    void refreshWithStoredToken();
    void startAuthCodePkce();
    void startDeviceCode();
    void exchangeAuthorizationCode(const QString& code);
    void pollDeviceCode();
    void handleTokenJson(const QByteArray& raw, bool expectRefreshToken);
    void storeRefreshToken(const QString& refreshToken);
    [[nodiscard]] std::optional<QString> loadRefreshToken() const;
    void clearRefreshToken();

    void fail(const QString& error);
};

} // namespace storepulse
