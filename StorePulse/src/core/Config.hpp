#pragma once

#include <QDate>
#include <QSettings>
#include <QString>
#include <optional>

namespace storepulse {

/// Non-secret app settings. Secrets/refresh tokens never live here.
class Config {
public:
    static Config& instance();

    /// "interactive" (default) or "client_credentials"
    QString authMode() const;
    void setAuthMode(const QString& mode);

    /// "common" | "consumers" | "organizations"
    QString accountAudience() const;
    void setAccountAudience(const QString& audience);

    /// Public-client app ID used for interactive Microsoft sign-in.
    QString interactiveClientId() const;
    void setInteractiveClientId(const QString& clientId);

    /// Entra tenant / app IDs for advanced client-credentials mode.
    QString tenantId() const;
    void setTenantId(const QString& tenantId);

    QString clientId() const;
    void setClientId(const QString& clientId);

    /// Optional calendar date when the client secret expires (UI warning only).
    std::optional<QDate> clientSecretExpiresOn() const;
    void setClientSecretExpiresOn(const std::optional<QDate>& date);

    int secretExpiryWarningDays() const;
    void setSecretExpiryWarningDays(int days);

    bool isSecretExpiringSoon() const;
    bool isSecretExpired() const;

    QString databasePath() const;
    void setDatabasePath(const QString& path);

    void sync();

private:
    Config();
    QSettings m_settings;
};

} // namespace storepulse
