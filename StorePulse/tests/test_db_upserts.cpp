#include "AcquisitionRepository.hpp"
#include "FailureHitRepository.hpp"
#include "SqliteDatabase.hpp"

#include <QDate>
#include <QTemporaryDir>
#include <QtTest>

using storepulse::AcquisitionRepository;
using storepulse::AcquisitionRow;
using storepulse::FailureHitRepository;
using storepulse::FailureHitRow;
using storepulse::SqliteDatabase;

class TestDbUpserts : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void acquisitionUpsertRestatesQuantity();
    void failureHitUpsertAccumulatesByNaturalKey();

private:
    QTemporaryDir m_temp;
    SqliteDatabase m_db;
};

void TestDbUpserts::initTestCase()
{
    QVERIFY(m_temp.isValid());
    const QString path = m_temp.filePath(QStringLiteral("test.sqlite"));
    QVERIFY2(m_db.open(path), qPrintable(m_db.lastError()));
}

void TestDbUpserts::acquisitionUpsertRestatesQuantity()
{
    AcquisitionRepository repo(m_db);

    AcquisitionRow row;
    row.storeId = QStringLiteral("9P0HGKHQ9W19");
    row.dateUtc = QStringLiteral("2026-11-10");
    row.granularity = QStringLiteral("daily");
    row.acquisitionType = QStringLiteral("Free");
    row.market = QStringLiteral("US");
    row.quantity = 10;
    row.source = QStringLiteral("analytics");

    QVERIFY2(repo.upsert(row), qPrintable(repo.lastError()));

    row.quantity = 14; // Microsoft restatement
    QVERIFY2(repo.upsertRestatementWindow(row.storeId,
                                          QDate(2026, 11, 8),
                                          QDate(2026, 11, 18),
                                          {row}),
             qPrintable(repo.lastError()));

    const qint64 sum = repo.sumQuantity(row.storeId, QDate(2026, 11, 10), QDate(2026, 11, 10));
    QCOMPARE(sum, 14);
}

void TestDbUpserts::failureHitUpsertAccumulatesByNaturalKey()
{
    FailureHitRepository repo(m_db);

    FailureHitRow row;
    row.storeId = QStringLiteral("9P0HGKHQ9W19");
    row.dateUtc = QStringLiteral("2026-11-17");
    row.failureName = QStringLiteral("AV in ModuleX");
    row.failureHash = QStringLiteral("abc123");
    row.failureStackHash = QStringLiteral("stack99");
    row.osVersion = QStringLiteral("10.0.26100");
    row.osArchitecture = QStringLiteral("ARM64");
    row.packageVersion = QStringLiteral("1.2.3.0");
    row.eventType = QStringLiteral("crash");
    row.eventCount = 3;
    row.cabId = QStringLiteral("cab-1");

    QVERIFY2(repo.upsert(row), qPrintable(repo.lastError()));
    row.eventCount = 7;
    QVERIFY2(repo.upsert(row), qPrintable(repo.lastError()));

    const qint64 total = repo.sumEvents(row.storeId, row.packageVersion,
                                        QDate(2026, 11, 17), QDate(2026, 11, 17));
    QCOMPARE(total, 7);
}

QTEST_MAIN(TestDbUpserts)
#include "test_db_upserts.moc"
