#pragma once

#include "AuthProvider.hpp"

#include <QNetworkAccessManager>
#include <QUrl>
#include <functional>
#include <optional>

namespace storepulse {

/// Advanced / headless mode: Entra app registration + client credentials.
/// Prefer InteractiveAuthClient for Hotmail / work-account sign-in.
class EntraAuthClient : public AuthProvider {
    Q_OBJECT
public:
    explicit EntraAuthClient(QNetworkAccessManager* nam, QObject* parent = nullptr);

    [[nodiscard]] AuthMode mode() const override { return AuthMode::ClientCredentials; }

    void setTenantId(const QString& tenantId);
    void setClientId(const QString& clientId);

    using SecretProvider = std::function<std::optional<QString>()>;
    void setSecretProvider(SecretProvider provider);
    void setTokenEndpointOverride(const QUrl& url);

    [[nodiscard]] bool hasCachedToken() const override;
    [[nodiscard]] std::optional<AccessToken> cachedToken() const override;
    void clearCachedToken() override;
    void requestAccessToken() override;
    [[nodiscard]] QString lastError() const override;

private slots:
    void onTokenReplyFinished();

private:
    QNetworkAccessManager* m_nam = nullptr;
    QString m_tenantId;
    QString m_clientId;
    SecretProvider m_secretProvider;
    QUrl m_endpointOverride;
    std::optional<AccessToken> m_cached;
    QString m_lastError;
    bool m_inFlight = false;

    [[nodiscard]] QUrl tokenEndpoint() const;
    [[nodiscard]] bool tokenStillValid(int skewSeconds = kRefreshSkewSeconds) const;
};

} // namespace storepulse
