#include "InteractiveAuthClient.hpp"

#include "CredentialManager.hpp"
#include "Logger.hpp"
#include "Pkce.hpp"

#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpSocket>
#include <QTimer>
#include <QUrlQuery>

namespace storepulse {

InteractiveAuthClient::InteractiveAuthClient(QNetworkAccessManager* nam, QObject* parent)
    : AuthProvider(parent)
    , m_nam(nam)
{
}

void InteractiveAuthClient::setClientId(const QString& clientId) { m_clientId = clientId; }
void InteractiveAuthClient::setAudience(AccountAudience audience) { m_audience = audience; }
void InteractiveAuthClient::setScope(const QString& scope) { m_scope = scope; }
void InteractiveAuthClient::setPreferredFlow(Flow flow) { m_preferredFlow = flow; }
void InteractiveAuthClient::setAuthorityOverride(const QUrl& authorizeBase)
{
    m_authorityOverride = authorizeBase;
}
void InteractiveAuthClient::setTokenEndpointOverride(const QUrl& tokenUrl)
{
    m_tokenOverride = tokenUrl;
}
void InteractiveAuthClient::setRefreshTokenTarget(const QString& targetName)
{
    m_refreshTarget = targetName;
}

QString InteractiveAuthClient::clientId() const { return m_clientId; }
AccountAudience InteractiveAuthClient::audience() const { return m_audience; }
QString InteractiveAuthClient::lastError() const { return m_lastError; }

bool InteractiveAuthClient::hasCachedToken() const { return tokenStillValid(); }

std::optional<AccessToken> InteractiveAuthClient::cachedToken() const
{
    if (!tokenStillValid()) {
        return std::nullopt;
    }
    return m_cached;
}

void InteractiveAuthClient::clearCachedToken() { m_cached.reset(); }

bool InteractiveAuthClient::tokenStillValid(int skewSeconds) const
{
    if (!m_cached || m_cached->value.isEmpty()) {
        return false;
    }
    return m_cached->expiresAtUtc > QDateTime::currentDateTimeUtc().addSecs(skewSeconds);
}

QUrl InteractiveAuthClient::authorityRoot() const
{
    if (m_authorityOverride.isValid()) {
        return m_authorityOverride;
    }
    return QUrl(QStringLiteral("https://login.microsoftonline.com/%1")
                    .arg(audienceTenantPath(m_audience)));
}

QUrl InteractiveAuthClient::authorizeUrl() const
{
    QUrl url = authorityRoot();
    QString path = url.path();
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }
    path += QStringLiteral("oauth2/v2.0/authorize");
    url.setPath(path);
    return url;
}

QUrl InteractiveAuthClient::tokenUrl() const
{
    if (m_tokenOverride.isValid()) {
        return m_tokenOverride;
    }
    QUrl url = authorityRoot();
    QString path = url.path();
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }
    path += QStringLiteral("oauth2/v2.0/token");
    url.setPath(path);
    return url;
}

bool InteractiveAuthClient::hasRefreshToken() const
{
    return loadRefreshToken().has_value();
}

std::optional<QString> InteractiveAuthClient::loadRefreshToken() const
{
    CredentialManager store(m_refreshTarget);
    if (auto token = store.readSecret()) {
        return token;
    }
    // Pre-rename fallback (Store Pulse → StoreVitals).
    CredentialManager legacy(QStringLiteral("StorePulse/MicrosoftRefreshToken"));
    if (auto token = legacy.readSecret()) {
        // Re-home under the new target so subsequent launches stay on StoreVitals.
        CredentialManager(m_refreshTarget).storeSecret(*token);
        return token;
    }
    return std::nullopt;
}

void InteractiveAuthClient::storeRefreshToken(const QString& refreshToken)
{
    CredentialManager store(m_refreshTarget);
    if (!store.storeSecret(refreshToken)) {
        Logger::error(QStringLiteral("Failed to persist refresh token in Credential Manager"));
    }
}

void InteractiveAuthClient::clearRefreshToken()
{
    CredentialManager store(m_refreshTarget);
    store.deleteSecret();
}

void InteractiveAuthClient::signOut()
{
    clearCachedToken();
    clearRefreshToken();
    Logger::info(QStringLiteral("Signed out — tokens cleared"));
}

void InteractiveAuthClient::fail(const QString& error)
{
    m_inFlight = false;
    m_lastError = error;
    Logger::error(error);
    emit tokenFailed(error);
}

void InteractiveAuthClient::requestAccessToken()
{
    m_lastError.clear();
    if (tokenStillValid()) {
        emit tokenReady(m_cached->value, m_cached->expiresAtUtc);
        return;
    }
    if (m_inFlight) {
        return;
    }
    if (hasRefreshToken()) {
        refreshWithStoredToken();
        return;
    }
    emit interactiveSignInRequired(
        QStringLiteral("Sign in with your Microsoft account (Hotmail, Outlook.com, or work)"));
    signInInteractively();
}

void InteractiveAuthClient::signInInteractively()
{
    m_lastError.clear();
    if (m_clientId.isEmpty()) {
        fail(QStringLiteral("Interactive client ID is not configured"));
        return;
    }
    if (m_preferredFlow == Flow::DeviceCode) {
        startDeviceCode();
    } else {
        startAuthCodePkce();
    }
}

void InteractiveAuthClient::refreshWithStoredToken()
{
    const auto refresh = loadRefreshToken();
    if (!refresh) {
        emit interactiveSignInRequired(QStringLiteral("Refresh token missing — sign in again"));
        signInInteractively();
        return;
    }

    m_inFlight = true;
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("client_id"), m_clientId);
    body.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("refresh_token"));
    body.addQueryItem(QStringLiteral("refresh_token"), *refresh);
    body.addQueryItem(QStringLiteral("scope"), m_scope);

    QNetworkRequest req(tokenUrl());
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/x-www-form-urlencoded"));

    Logger::info(QStringLiteral("Refreshing Microsoft access token"));
    auto* reply = m_nam->post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            clearRefreshToken();
            fail(QStringLiteral("Refresh failed — please sign in again (%1)")
                     .arg(reply->errorString()));
            emit interactiveSignInRequired(m_lastError);
            return;
        }
        handleTokenJson(reply->readAll(), false);
    });
}

void InteractiveAuthClient::startAuthCodePkce()
{
    if (m_redirectServer.isListening()) {
        m_redirectServer.close();
    }

    // Fixed loopback port so Entra redirect URI can match exactly (MSA is strict).
    // Register under Mobile and desktop: http://localhost:8400
    constexpr quint16 kPreferredPort = 8400;
    bool listening = false;
    for (quint16 port = kPreferredPort; port < kPreferredPort + 20; ++port) {
        if (m_redirectServer.listen(QHostAddress::LocalHost, port)) {
            listening = true;
            break;
        }
    }
    if (!listening) {
        fail(QStringLiteral("Unable to start local redirect listener on ports 8400–8419"));
        return;
    }

    m_pkceVerifier = pkce::generateCodeVerifier();
    m_oauthState = pkce::generateState();
    // No trailing slash — must match the Mobile/desktop redirect URI in Entra exactly.
    m_redirectUri = QUrl(QStringLiteral("http://localhost:%1").arg(m_redirectServer.serverPort()));

    QUrl auth = authorizeUrl();
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("client_id"), m_clientId);
    q.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    q.addQueryItem(QStringLiteral("redirect_uri"), m_redirectUri.toString());
    q.addQueryItem(QStringLiteral("response_mode"), QStringLiteral("query"));
    q.addQueryItem(QStringLiteral("scope"), m_scope);
    q.addQueryItem(QStringLiteral("state"), m_oauthState);
    q.addQueryItem(QStringLiteral("code_challenge"), pkce::challengeS256(m_pkceVerifier));
    q.addQueryItem(QStringLiteral("code_challenge_method"), QStringLiteral("S256"));
    // Force account picker so Hotmail vs work is explicit.
    q.addQueryItem(QStringLiteral("prompt"), QStringLiteral("select_account"));
    auth.setQuery(q);

    m_inFlight = true;
    Logger::info(QStringLiteral("Opening browser for Microsoft sign-in (MSA or work account)"));

    connect(&m_redirectServer, &QTcpServer::newConnection, this, [this]() {
        while (auto* sock = m_redirectServer.nextPendingConnection()) {
            connect(sock, &QTcpSocket::readyRead, this, [this, sock]() {
                const QByteArray req = sock->readAll();
                const QString line = QString::fromUtf8(req).section(QLatin1Char('\n'), 0, 0);
                // GET /?code=...&state=... HTTP/1.1
                const QString path = line.section(QLatin1Char(' '), 1, 1);
                const QUrl hit(QStringLiteral("http://local") + path);
                const QUrlQuery query(hit);

                const QString html = QStringLiteral(
                    "<html><body style='font-family:Segoe UI,sans-serif;padding:2rem'>"
                    "<h2>StoreVitals</h2><p>Sign-in complete. You can close this window.</p>"
                    "</body></html>");
                QByteArray resp;
                resp += "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n";
                resp += "Content-Length: " + QByteArray::number(html.toUtf8().size()) + "\r\n";
                resp += "Connection: close\r\n\r\n";
                resp += html.toUtf8();
                sock->write(resp);
                sock->disconnectFromHost();
                m_redirectServer.close();

                if (query.hasQueryItem(QStringLiteral("error"))) {
                    fail(QStringLiteral("Sign-in error: %1 (%2)")
                             .arg(query.queryItemValue(QStringLiteral("error")),
                                  query.queryItemValue(QStringLiteral("error_description"))));
                    return;
                }
                if (query.queryItemValue(QStringLiteral("state")) != m_oauthState) {
                    fail(QStringLiteral("OAuth state mismatch — aborting sign-in"));
                    return;
                }
                const QString code = query.queryItemValue(QStringLiteral("code"));
                if (code.isEmpty()) {
                    fail(QStringLiteral("Authorization code missing from redirect"));
                    return;
                }
                exchangeAuthorizationCode(code);
            });
        }
    });

    if (!QDesktopServices::openUrl(auth)) {
        fail(QStringLiteral("Could not open the system browser for Microsoft sign-in"));
        m_redirectServer.close();
    }
}

void InteractiveAuthClient::exchangeAuthorizationCode(const QString& code)
{
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("client_id"), m_clientId);
    body.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
    body.addQueryItem(QStringLiteral("code"), code);
    body.addQueryItem(QStringLiteral("redirect_uri"), m_redirectUri.toString());
    body.addQueryItem(QStringLiteral("code_verifier"), m_pkceVerifier);
    body.addQueryItem(QStringLiteral("scope"), m_scope);

    QNetworkRequest req(tokenUrl());
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/x-www-form-urlencoded"));

    auto* reply = m_nam->post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            fail(QStringLiteral("Token exchange failed: %1").arg(reply->errorString()));
            return;
        }
        handleTokenJson(reply->readAll(), true);
    });
}

void InteractiveAuthClient::startDeviceCode()
{
    m_inFlight = true;

    QUrl dc = authorityRoot();
    QString path = dc.path();
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }
    path += QStringLiteral("oauth2/v2.0/devicecode");
    dc.setPath(path);

    QUrlQuery body;
    body.addQueryItem(QStringLiteral("client_id"), m_clientId);
    body.addQueryItem(QStringLiteral("scope"), m_scope);

    QNetworkRequest req(dc);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/x-www-form-urlencoded"));

    auto* reply = m_nam->post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            fail(QStringLiteral("Device code request failed: %1").arg(reply->errorString()));
            return;
        }
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        m_deviceCode = obj.value(QStringLiteral("device_code")).toString();
        const QString userCode = obj.value(QStringLiteral("user_code")).toString();
        const QString verifyUrl = obj.value(QStringLiteral("verification_uri")).toString();
        m_deviceInterval = QString::number(obj.value(QStringLiteral("interval")).toInt(5));
        m_deviceExpiresIn = obj.value(QStringLiteral("expires_in")).toInt(900);

        if (m_deviceCode.isEmpty() || userCode.isEmpty()) {
            fail(QStringLiteral("Device code response incomplete"));
            return;
        }

        emit deviceCodeReceived(userCode, verifyUrl);
        Logger::info(QStringLiteral("Device sign-in: visit %1 and enter %2")
                         .arg(verifyUrl, userCode));
        QDesktopServices::openUrl(QUrl(verifyUrl));
        pollDeviceCode();
    });
}

void InteractiveAuthClient::pollDeviceCode()
{
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("grant_type"),
                      QStringLiteral("urn:ietf:params:oauth:grant-type:device_code"));
    body.addQueryItem(QStringLiteral("client_id"), m_clientId);
    body.addQueryItem(QStringLiteral("device_code"), m_deviceCode);

    QNetworkRequest req(tokenUrl());
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/x-www-form-urlencoded"));

    auto* reply = m_nam->post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray raw = reply->readAll();
        const QJsonObject obj = QJsonDocument::fromJson(raw).object();
        const QString err = obj.value(QStringLiteral("error")).toString();

        if (err == QLatin1String("authorization_pending")
            || err == QLatin1String("slow_down")) {
            int interval = m_deviceInterval.toInt();
            if (err == QLatin1String("slow_down")) {
                interval += 5;
            }
            QTimer::singleShot(std::max(3, interval) * 1000, this, [this]() { pollDeviceCode(); });
            return;
        }

        if (reply->error() != QNetworkReply::NoError || !err.isEmpty()) {
            fail(QStringLiteral("Device sign-in failed: %1")
                     .arg(err.isEmpty() ? reply->errorString() : err));
            return;
        }

        handleTokenJson(raw, true);
    });
}

void InteractiveAuthClient::handleTokenJson(const QByteArray& raw, bool /*expectRefreshToken*/)
{
    const QJsonObject obj = QJsonDocument::fromJson(raw).object();
    const QString access = obj.value(QStringLiteral("access_token")).toString();
    const QString refresh = obj.value(QStringLiteral("refresh_token")).toString();
    const int expiresIn = obj.value(QStringLiteral("expires_in")).toInt(3600);

    if (access.isEmpty()) {
        const QString err = obj.value(QStringLiteral("error")).toString();
        const QString desc = obj.value(QStringLiteral("error_description")).toString();
        fail(QStringLiteral("Token response missing access_token (%1: %2)").arg(err, desc));
        return;
    }

    if (!refresh.isEmpty()) {
        storeRefreshToken(refresh);
    }

    AccessToken token;
    token.value = access;
    token.expiresAtUtc = QDateTime::currentDateTimeUtc().addSecs(expiresIn);
    m_cached = token;
    m_inFlight = false;

    Logger::info(QStringLiteral("Microsoft interactive token cached until %1")
                     .arg(token.expiresAtUtc.toString(Qt::ISODate)));
    emit tokenReady(token.value, token.expiresAtUtc);
}

} // namespace storepulse
