#include "AnalyticsParsers.hpp"
#include "RateLimiter.hpp"

#include <QFile>
#include <QtTest>

using storepulse::AnalyticsParsers::parseAcquisitions;
using storepulse::AnalyticsParsers::parseFailureHits;
using storepulse::AnalyticsParsers::parsePage;
using storepulse::RateLimiter;

class TestAnalyticsParsers : public QObject {
    Q_OBJECT

private slots:
    void parseAcquisitionPageAndNextLink();
    void parseFailureHitsPayload();
    void exponentialBackoffCaps();
};

static QByteArray loadFixture(const QString& name)
{
    QFile f(QStringLiteral(":/fixtures/%1").arg(name));
    if (f.open(QIODevice::ReadOnly)) {
        return f.readAll();
    }
    // Fallback to source tree relative path when resources aren't embedded.
    QFile disk(QStringLiteral("%1/fixtures/%2").arg(QStringLiteral(TEST_SOURCE_DIR), name));
    if (!disk.open(QIODevice::ReadOnly)) {
        return {};
    }
    return disk.readAll();
}

void TestAnalyticsParsers::parseAcquisitionPageAndNextLink()
{
    const QByteArray json = loadFixture(QStringLiteral("acquisitions_page1.json"));
    QVERIFY2(!json.isEmpty(), "missing acquisitions_page1.json fixture");
    const auto page = parsePage(json);
    QCOMPARE(page.values.size(), 2);
    QVERIFY(page.nextLink.contains(QStringLiteral("skip=2")));

    const auto rows = parseAcquisitions(json, QStringLiteral("9P0HGKHQ9W19"));
    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows[0].market, QStringLiteral("US"));
    QCOMPARE(rows[0].quantity, 10);
    QCOMPARE(rows[1].market, QStringLiteral("GB"));
    QCOMPARE(rows[1].quantity, 4);
}

void TestAnalyticsParsers::parseFailureHitsPayload()
{
    const QByteArray json = loadFixture(QStringLiteral("failurehits.json"));
    QVERIFY2(!json.isEmpty(), "missing failurehits.json fixture");
    const auto rows = parseFailureHits(json, QStringLiteral("9P0HGKHQ9W19"));
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows[0].osArchitecture, QStringLiteral("ARM64"));
    QCOMPARE(rows[0].eventCount, 5);
    QCOMPARE(rows[0].failureHash, QStringLiteral("abc123"));
}

void TestAnalyticsParsers::exponentialBackoffCaps()
{
    QCOMPARE(RateLimiter::exponentialBackoffSeconds(0), 2);
    QCOMPARE(RateLimiter::exponentialBackoffSeconds(1), 4);
    QCOMPARE(RateLimiter::exponentialBackoffSeconds(2), 8);
    QCOMPARE(RateLimiter::exponentialBackoffSeconds(20, 2, 300), 300);
}

QTEST_MAIN(TestAnalyticsParsers)
#include "test_analytics_parsers.moc"
