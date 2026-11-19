#include "AcquisitionRepository.hpp"
#include "AnalyticsSyncService.hpp"
#include "EntraAuthClient.hpp"
#include "InteractiveAuthClient.hpp"
#include "PartnerCenterClient.hpp"
#include "Pkce.hpp"
#include "SqliteDatabase.hpp"

#include <QDate>
#include <QNetworkAccessManager>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

using namespace storepulse;

/// Minimal HTTP stub that answers token + analytics routes.
class MockHttpServer : public QObject {
    Q_OBJECT
public:
    explicit MockHttpServer(QObject* parent = nullptr)
        : QObject(parent)
    {
        connect(&m_server, &QTcpServer::newConnection, this, &MockHttpServer::onNewConnection);
    }

    bool listen()
    {
        return m_server.listen(QHostAddress::LocalHost, 0);
    }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1").arg(m_server.serverPort()));
    }

    QUrl tokenUrl() const
    {
        QUrl u = baseUrl();
        u.setPath(QStringLiteral("/oauth2/token"));
        return u;
    }

    QUrl analyticsBase() const
    {
        QUrl u = baseUrl();
        u.setPath(QStringLiteral("/v1.0/my/analytics"));
        return u;
    }

    int tokenHits = 0;
    int acqHits = 0;
    int failHits = 0;

private:
    QTcpServer m_server;

    void onNewConnection()
    {
        while (QTcpSocket* sock = m_server.nextPendingConnection()) {
            connect(sock, &QTcpSocket::readyRead, this, [this, sock]() {
                const QByteArray req = sock->readAll();
                const QString firstLine = QString::fromUtf8(req).section(QLatin1Char('\n'), 0, 0);

                QByteArray body;
                int status = 200;

                if (firstLine.contains(QStringLiteral("/oauth2/token"))
                || firstLine.contains(QStringLiteral("/oauth2/v2.0/token"))) {
                    ++tokenHits;
                    body = R"({"token_type":"Bearer","expires_in":3600,"access_token":"mock-token"})";
                } else if (firstLine.contains(QStringLiteral("appacquisitions"))) {
                    ++acqHits;
                    if (firstLine.contains(QStringLiteral("skip=2"))) {
                        body = R"({"Value":[{"date":"2026-11-12","acquisitionType":"Paid","market":"US","deviceType":"PC","acquisitionQuantity":2}]})";
                    } else {
                        body = R"({"Value":[{"date":"2026-11-10","acquisitionType":"Free","market":"US","deviceType":"PC","acquisitionQuantity":10},{"date":"2026-11-11","acquisitionType":"Free","market":"GB","deviceType":"PC","acquisitionQuantity":4}],"@nextLink":"/v1.0/my/analytics/appacquisitions?applicationId=9P0HGKHQ9W19&skip=2"})";
                    }
                } else if (firstLine.contains(QStringLiteral("failurehits"))) {
                    ++failHits;
                    body = R"({"value":[{"date":"2026-11-17","failureName":"AV","failureHash":"h1","failureStackHash":"s1","osVersion":"10.0","osArchitecture":"x64","packageVersion":"1.0.0.0","eventType":"crash","eventCount":3,"cabId":"c1"}]})";
                } else {
                    status = 404;
                    body = R"({"error":"not found"})";
                }

                QByteArray response;
                response += "HTTP/1.1 " + QByteArray::number(status) + " OK\r\n";
                response += "Content-Type: application/json\r\n";
                response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
                response += "Connection: close\r\n\r\n";
                response += body;
                sock->write(response);
                sock->disconnectFromHost();
            });
        }
    }
};

class TestApiClient : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void authCachesToken();
    void pkceChallengeIsStableAndUrlSafe();
    void syncUpsertsFromMockApi();

private:
    MockHttpServer m_server;
    QNetworkAccessManager m_nam;
};

void TestApiClient::initTestCase()
{
    QVERIFY(m_server.listen());
}

void TestApiClient::authCachesToken()
{
    EntraAuthClient auth(&m_nam);
    auth.setTenantId(QStringLiteral("tenant"));
    auth.setClientId(QStringLiteral("client"));
    auth.setSecretProvider([] { return std::optional<QString>(QStringLiteral("secret")); });
    auth.setTokenEndpointOverride(m_server.tokenUrl());

    QSignalSpy ready(&auth, &AuthProvider::tokenReady);
    QSignalSpy failed(&auth, &AuthProvider::tokenFailed);

    auth.requestAccessToken();
    QVERIFY(ready.wait(5000));
    QCOMPARE(failed.count(), 0);
    QCOMPARE(m_server.tokenHits, 1);
    QVERIFY(auth.hasCachedToken());

    // Second call should hit cache synchronously (no new HTTP).
    auth.requestAccessToken();
    QCOMPARE(ready.count(), 2);
    QCOMPARE(m_server.tokenHits, 1);
}

void TestApiClient::pkceChallengeIsStableAndUrlSafe()
{
    const QString verifier = QStringLiteral("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk");
    const QString challenge = storepulse::pkce::challengeS256(verifier);
    // RFC 7636 appendix B known answer
    QCOMPARE(challenge, QStringLiteral("E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"));
    QVERIFY(!challenge.contains(QLatin1Char('+')));
    QVERIFY(!challenge.contains(QLatin1Char('/')));
    QVERIFY(!challenge.contains(QLatin1Char('=')));
}

void TestApiClient::syncUpsertsFromMockApi()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    SqliteDatabase db;
    QVERIFY2(db.open(temp.filePath(QStringLiteral("sync.sqlite"))), qPrintable(db.lastError()));

    EntraAuthClient auth(&m_nam);
    auth.setTenantId(QStringLiteral("tenant"));
    auth.setClientId(QStringLiteral("client"));
    auth.setSecretProvider([] { return std::optional<QString>(QStringLiteral("secret")); });
    auth.setTokenEndpointOverride(m_server.tokenUrl());

    PartnerCenterClient client(auth, &m_nam);
    client.setApiBaseOverride(m_server.analyticsBase());

    AnalyticsSyncService sync(client, db);
    QSignalSpy finished(&sync, &AnalyticsSyncService::syncFinished);

    const int tokensBefore = m_server.tokenHits;
    sync.syncApp(QStringLiteral("9P0HGKHQ9W19"));
    QVERIFY(finished.wait(10000));
    QCOMPARE(finished.first().at(1).toBool(), true);

    QVERIFY(m_server.acqHits >= 2); // page1 + page2
    QCOMPARE(m_server.failHits, 1);
    QVERIFY(m_server.tokenHits >= tokensBefore + 1);

    AcquisitionRepository repo(db);
    const qint64 sum = repo.sumQuantity(QStringLiteral("9P0HGKHQ9W19"),
                                        QDate(2026, 11, 10),
                                        QDate(2026, 11, 12));
    QCOMPARE(sum, 16); // 10 + 4 + 2
}

QTEST_MAIN(TestApiClient)
#include "test_api_client.moc"
