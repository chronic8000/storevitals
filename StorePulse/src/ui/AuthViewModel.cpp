#include "AuthViewModel.hpp"

#include "Config.hpp"
#include "CredentialManager.hpp"
#include "Logger.hpp"

#include <QDesktopServices>
#include <QUrl>

namespace storepulse {

AuthViewModel::AuthViewModel(QNetworkAccessManager* nam, QObject* parent)
    : QObject(parent)
    , m_nam(nam)
    , m_client(std::make_unique<InteractiveAuthClient>(nam))
{
    loadFromConfig();
    applyClientConfig();
    wireClient();

    if (m_client->hasRefreshToken()) {
        setStatus(QStringLiteral("Signed in — refresh token on this PC"));
    } else if (!hasClientId()) {
        setStatus(QStringLiteral("Add an Application (client) ID, then sign in with Microsoft"));
    } else {
        setStatus(QStringLiteral("Ready — sign in with Hotmail, Outlook.com, or a work account"));
    }
}

void AuthViewModel::loadFromConfig()
{
    auto& cfg = Config::instance();
    m_interactiveClientId = cfg.interactiveClientId();
    m_audience = cfg.accountAudience();
    if (m_audience.isEmpty()) {
        m_audience = QStringLiteral("common");
    }
    m_tenantId = cfg.tenantId();
    m_apiClientId = cfg.clientId();
    if (m_apiClientId.isEmpty()) {
        m_apiClientId = m_interactiveClientId;
    }
}

AccountAudience AuthViewModel::parseAudience(const QString& value) const
{
    if (value == QLatin1String("consumers")) {
        return AccountAudience::Consumers;
    }
    if (value == QLatin1String("organizations")) {
        return AccountAudience::Organizations;
    }
    return AccountAudience::Common;
}

void AuthViewModel::applyClientConfig()
{
    m_client->setClientId(m_interactiveClientId.trimmed());
    m_client->setAudience(parseAudience(m_audience));
}

void AuthViewModel::wireClient()
{
    connect(m_client.get(), &AuthProvider::tokenReady, this,
            [this](const QString&, const QDateTime&) {
                setBusy(false);
                m_deviceUserCode.clear();
                m_deviceVerificationUrl.clear();
                emit deviceCodeChanged();
                setError({});
                setStatus(QStringLiteral("Signed in with Microsoft"));
                emit signedInChanged();
                emit signInSucceeded();
            });

    connect(m_client.get(), &AuthProvider::tokenFailed, this, [this](const QString& error) {
        setBusy(false);
        setError(error);
        setStatus(QStringLiteral("Sign-in failed"));
        emit signInFailed(error);
    });

    connect(m_client.get(), &AuthProvider::deviceCodeReceived, this,
            [this](const QString& userCode, const QString& url) {
                m_deviceUserCode = userCode;
                m_deviceVerificationUrl = url;
                emit deviceCodeChanged();
                setStatus(QStringLiteral("Enter this code in your browser to finish sign-in"));
            });

    connect(m_client.get(), &AuthProvider::interactiveSignInRequired, this,
            [this](const QString& message) { setStatus(message); });
}

bool AuthViewModel::signedIn() const
{
    return m_client && (m_client->hasCachedToken() || m_client->hasRefreshToken());
}

bool AuthViewModel::hasApiSecret() const
{
    CredentialManager creds;
    return creds.hasSecret();
}

void AuthViewModel::setInteractiveClientId(const QString& clientId)
{
    if (m_interactiveClientId == clientId) {
        return;
    }
    m_interactiveClientId = clientId;
    emit interactiveClientIdChanged();
}

void AuthViewModel::setAudience(const QString& audience)
{
    if (m_audience == audience) {
        return;
    }
    m_audience = audience;
    emit audienceChanged();
}

void AuthViewModel::setTenantId(const QString& tenantId)
{
    if (m_tenantId == tenantId) {
        return;
    }
    m_tenantId = tenantId;
    emit apiSettingsChanged();
}

void AuthViewModel::setApiClientId(const QString& clientId)
{
    if (m_apiClientId == clientId) {
        return;
    }
    m_apiClientId = clientId;
    emit apiSettingsChanged();
}

void AuthViewModel::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
}

void AuthViewModel::setStatus(const QString& message)
{
    if (m_statusMessage == message) {
        return;
    }
    m_statusMessage = message;
    emit statusMessageChanged();
}

void AuthViewModel::setError(const QString& error)
{
    if (m_lastError == error) {
        return;
    }
    m_lastError = error;
    emit lastErrorChanged();
}

void AuthViewModel::saveSettings()
{
    auto& cfg = Config::instance();
    cfg.setAuthMode(QStringLiteral("interactive"));
    cfg.setInteractiveClientId(m_interactiveClientId.trimmed());
    cfg.setAccountAudience(m_audience);
    cfg.sync();
    applyClientConfig();
    setStatus(QStringLiteral("Sign-in settings saved"));
}

void AuthViewModel::saveApiSettings()
{
    auto& cfg = Config::instance();
    cfg.setTenantId(m_tenantId.trimmed());
    cfg.setClientId(m_apiClientId.trimmed());
    cfg.sync();
    setStatus(QStringLiteral("API settings saved (secret stored separately in Credential Manager)"));
    emit apiCredentialsSaved();
}

bool AuthViewModel::storeApiClientSecret(const QString& secret)
{
    CredentialManager creds;
    if (!creds.storeSecret(secret)) {
        setError(creds.lastError().isEmpty() ? QStringLiteral("Failed to store client secret")
                                             : creds.lastError());
        return false;
    }
    setError({});
    setStatus(QStringLiteral("Client secret stored in Windows Credential Manager"));
    emit apiSecretChanged();
    emit apiCredentialsSaved();
    return true;
}

void AuthViewModel::clearApiClientSecret()
{
    CredentialManager creds;
    creds.deleteSecret();
    emit apiSecretChanged();
    setStatus(QStringLiteral("Client secret removed"));
    emit apiCredentialsSaved();
}

void AuthViewModel::signInBrowser()
{
    saveSettings();
    if (!hasClientId()) {
        setError(QStringLiteral("Application (client) ID is required before sign-in"));
        return;
    }
    setError({});
    setBusy(true);
    setStatus(QStringLiteral("Opening browser for Microsoft sign-in…"));
    m_client->setPreferredFlow(InteractiveAuthClient::Flow::AuthorizationCodePkce);
    m_client->signInInteractively();
}

void AuthViewModel::signInDeviceCode()
{
    saveSettings();
    if (!hasClientId()) {
        setError(QStringLiteral("Application (client) ID is required before sign-in"));
        return;
    }
    setError({});
    setBusy(true);
    setStatus(QStringLiteral("Starting device-code sign-in…"));
    m_client->setPreferredFlow(InteractiveAuthClient::Flow::DeviceCode);
    m_client->signInInteractively();
}

void AuthViewModel::signOut()
{
    m_client->signOut();
    m_deviceUserCode.clear();
    m_deviceVerificationUrl.clear();
    emit deviceCodeChanged();
    setError({});
    setStatus(QStringLiteral("Signed out"));
    emit signedInChanged();
}

void AuthViewModel::openRegistrationGuide()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral(
        "https://learn.microsoft.com/en-us/entra/identity-platform/quickstart-register-app")));
}

void AuthViewModel::openAzurePortalAppRegistration()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral(
        "https://portal.azure.com/#blade/Microsoft_AAD_RegisteredApps/ApplicationsListBlade")));
}

void AuthViewModel::openPartnerCenterUserManagement()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral(
        "https://partner.microsoft.com/dashboard/account/v3/usermanagement")));
}

void AuthViewModel::openPartnerCenterTenants()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral(
        "https://partner.microsoft.com/dashboard/account/v3/tenant")));
}

} // namespace storepulse
