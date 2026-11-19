#pragma once

#include "InteractiveAuthClient.hpp"

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <memory>

namespace storepulse {

/// QML-facing auth + settings for Microsoft sign-in and Dev Center API credentials.
class AuthViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool signedIn READ signedIn NOTIFY signedInChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString interactiveClientId READ interactiveClientId WRITE setInteractiveClientId NOTIFY interactiveClientIdChanged)
    Q_PROPERTY(QString audience READ audience WRITE setAudience NOTIFY audienceChanged)
    Q_PROPERTY(QString deviceUserCode READ deviceUserCode NOTIFY deviceCodeChanged)
    Q_PROPERTY(QString deviceVerificationUrl READ deviceVerificationUrl NOTIFY deviceCodeChanged)
    Q_PROPERTY(bool hasClientId READ hasClientId NOTIFY interactiveClientIdChanged)
    Q_PROPERTY(QString tenantId READ tenantId WRITE setTenantId NOTIFY apiSettingsChanged)
    Q_PROPERTY(QString apiClientId READ apiClientId WRITE setApiClientId NOTIFY apiSettingsChanged)
    Q_PROPERTY(bool hasApiSecret READ hasApiSecret NOTIFY apiSecretChanged)

public:
    explicit AuthViewModel(QNetworkAccessManager* nam, QObject* parent = nullptr);

    [[nodiscard]] InteractiveAuthClient* client() const { return m_client.get(); }

    [[nodiscard]] bool signedIn() const;
    [[nodiscard]] bool busy() const { return m_busy; }
    [[nodiscard]] QString statusMessage() const { return m_statusMessage; }
    [[nodiscard]] QString lastError() const { return m_lastError; }
    [[nodiscard]] QString interactiveClientId() const { return m_interactiveClientId; }
    [[nodiscard]] QString audience() const { return m_audience; }
    [[nodiscard]] QString deviceUserCode() const { return m_deviceUserCode; }
    [[nodiscard]] QString deviceVerificationUrl() const { return m_deviceVerificationUrl; }
    [[nodiscard]] bool hasClientId() const { return !m_interactiveClientId.trimmed().isEmpty(); }
    [[nodiscard]] QString tenantId() const { return m_tenantId; }
    [[nodiscard]] QString apiClientId() const { return m_apiClientId; }
    [[nodiscard]] bool hasApiSecret() const;

    void setInteractiveClientId(const QString& clientId);
    void setAudience(const QString& audience);
    void setTenantId(const QString& tenantId);
    void setApiClientId(const QString& clientId);

    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void saveApiSettings();
    Q_INVOKABLE bool storeApiClientSecret(const QString& secret);
    Q_INVOKABLE void clearApiClientSecret();
    Q_INVOKABLE void signInBrowser();
    Q_INVOKABLE void signInDeviceCode();
    Q_INVOKABLE void signOut();
    Q_INVOKABLE void openRegistrationGuide();
    Q_INVOKABLE void openAzurePortalAppRegistration();
    Q_INVOKABLE void openPartnerCenterUserManagement();
    Q_INVOKABLE void openPartnerCenterTenants();

signals:
    void signedInChanged();
    void busyChanged();
    void statusMessageChanged();
    void lastErrorChanged();
    void interactiveClientIdChanged();
    void audienceChanged();
    void deviceCodeChanged();
    void apiSettingsChanged();
    void apiSecretChanged();
    void signInSucceeded();
    void signInFailed(const QString& error);
    void apiCredentialsSaved();

private:
    QNetworkAccessManager* m_nam = nullptr;
    std::unique_ptr<InteractiveAuthClient> m_client;
    QString m_interactiveClientId;
    QString m_audience;
    QString m_tenantId;
    QString m_apiClientId;
    QString m_statusMessage;
    QString m_lastError;
    QString m_deviceUserCode;
    QString m_deviceVerificationUrl;
    bool m_busy = false;

    void loadFromConfig();
    void applyClientConfig();
    void setBusy(bool busy);
    void setStatus(const QString& message);
    void setError(const QString& error);
    void wireClient();
    [[nodiscard]] AccountAudience parseAudience(const QString& value) const;
};

} // namespace storepulse
