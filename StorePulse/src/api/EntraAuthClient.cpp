#include "EntraAuthClient.hpp"

#include "CredentialManager.hpp"
#include "Logger.hpp"
#include "TokenInspector.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace storepulse {

EntraAuthClient::EntraAuthClient(QNetworkAccessManager* nam, QObject* parent)
    : AuthProvider(parent)
    , m_nam(nam)
{
    m_secretProvider = []() -> std::optional<QString> {
        CredentialManager creds;
        auto secret = creds.readSecret();
        if (secret) {
            *secret = secret->trimmed();
        }
        return secret;
    };
}

void EntraAuthClient::setTenantId(const QString& tenantId) { m_tenantId = tenantId.trimmed(); }
void EntraAuthClient::setClientId(const QString& clientId) { m_clientId = clientId.trimmed(); }

void EntraAuthClient::setSecretProvider(SecretProvider provider)
{
    m_secretProvider = std::move(provider);
}

void EntraAuthClient::setTokenEndpointOverride(const QUrl& url)
{
    m_endpointOverride = url;
}

bool EntraAuthClient::hasCachedToken() const
{
    return tokenStillValid();
}

std::optional<AccessToken> EntraAuthClient::cachedToken() const
{
    if (!tokenStillValid()) {
        return std::nullopt;
    }
    return m_cached;
}

void EntraAuthClient::clearCachedToken()
{
    m_cached.reset();
}

QString EntraAuthClient::lastError() const
{
    return m_lastError;
}

QUrl EntraAuthClient::tokenEndpoint() const
{
    if (m_endpointOverride.isValid()) {
        return m_endpointOverride;
    }
    // Official Store analytics docs use the v1 token endpoint + resource=.
    return QUrl(QStringLiteral("https://login.microsoftonline.com/%1/oauth2/token")
                    .arg(m_tenantId));
}

bool EntraAuthClient::tokenStillValid(int skewSeconds) const
{
    if (!m_cached || m_cached->value.isEmpty()) {
        return false;
    }
    const QDateTime threshold = QDateTime::currentDateTimeUtc().addSecs(skewSeconds);
    return m_cached->expiresAtUtc > threshold;
}

void EntraAuthClient::requestAccessToken()
{
    m_lastError.clear();

    if (tokenStillValid()) {
        emit tokenReady(m_cached->value, m_cached->expiresAtUtc);
        return;
    }

    if (m_inFlight) {
        return;
    }

    if (m_tenantId.isEmpty() || m_clientId.isEmpty()) {
        m_lastError = QStringLiteral("tenantId and clientId are required for client-credentials mode");
        emit tokenFailed(m_lastError);
        return;
    }

    const QString tenantLower = m_tenantId.toLower();
    if (tenantLower == QLatin1String("common") || tenantLower == QLatin1String("consumers")
        || tenantLower == QLatin1String("organizations")) {
        m_lastError = QStringLiteral(
            "Client credentials require your Directory (tenant) ID GUID, not common/consumers");
        emit tokenFailed(m_lastError);
        return;
    }

    const auto secret = m_secretProvider ? m_secretProvider() : std::nullopt;
    if (!secret || secret->isEmpty()) {
        m_lastError = QStringLiteral("Client secret unavailable from Credential Manager");
        emit tokenFailed(m_lastError);
        return;
    }

    // Microsoft Store analytics documented shape:
    // grant_type=client_credentials&resource=https://manage.devcenter.microsoft.com
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("client_credentials"));
    body.addQueryItem(QStringLiteral("client_id"), m_clientId);
    body.addQueryItem(QStringLiteral("client_secret"), *secret);
    body.addQueryItem(QStringLiteral("resource"), QLatin1String(kDevCenterResource));

    QNetworkRequest req(tokenEndpoint());
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/x-www-form-urlencoded"));

    m_inFlight = true;
    Logger::info(QStringLiteral("Requesting Entra access token (client credentials v1/resource) for tenant %1")
                     .arg(m_tenantId));
    QNetworkReply* reply = m_nam->post(req, body.query(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, &EntraAuthClient::onTokenReplyFinished);
}

void EntraAuthClient::onTokenReplyFinished()
{
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    m_inFlight = false;
    if (!reply) {
        return;
    }
    reply->deleteLater();

    const QByteArray raw = reply->readAll();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QJsonObject obj = QJsonDocument::fromJson(raw).object();

    const QString aadError = obj.value(QStringLiteral("error")).toString();
    const QString aadDesc = obj.value(QStringLiteral("error_description")).toString();

    if (reply->error() != QNetworkReply::NoError || status >= 400 || !aadError.isEmpty()) {
        if (!aadError.isEmpty()) {
            m_lastError = QStringLiteral("Token request failed (%1): %2 - %3")
                              .arg(status)
                              .arg(aadError, aadDesc.left(300));
        } else {
            m_lastError = QStringLiteral("Token request failed (%1): %2")
                              .arg(status)
                              .arg(reply->errorString());
        }
        Logger::error(m_lastError);
        emit tokenFailed(m_lastError);
        return;
    }

    const QString accessToken = obj.value(QStringLiteral("access_token")).toString();
    const int expiresIn = obj.value(QStringLiteral("expires_in")).toInt(3600);

    if (accessToken.isEmpty()) {
        m_lastError = QStringLiteral("Token response missing access_token");
        emit tokenFailed(m_lastError);
        return;
    }

    AccessToken token;
    token.value = accessToken;
    token.expiresAtUtc = QDateTime::currentDateTimeUtc().addSecs(expiresIn);
    m_cached = token;

    const QString aud = TokenInspector::audience(token.value);
    Logger::info(QStringLiteral("Entra access token cached until %1 (aud=%2)")
                     .arg(token.expiresAtUtc.toString(Qt::ISODate),
                          aud.isEmpty() ? QStringLiteral("unknown") : aud));
    emit tokenReady(token.value, token.expiresAtUtc);
}

} // namespace storepulse
