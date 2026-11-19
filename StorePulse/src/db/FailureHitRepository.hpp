#pragma once

#include "Models.hpp"
#include "SqliteDatabase.hpp"

#include <QDate>
#include <QVector>

namespace storepulse {

class FailureHitRepository {
public:
    explicit FailureHitRepository(SqliteDatabase& db);

    [[nodiscard]] bool upsert(const FailureHitRow& row);
    [[nodiscard]] bool upsertBatch(const QVector<FailureHitRow>& rows);

    /// Total event_count for a store/package over [from,to] — used by crash-spike alerts.
    [[nodiscard]] qint64 sumEvents(const QString& storeId,
                                   const QString& packageVersion,
                                   const QDate& fromUtc,
                                   const QDate& toUtc) const;

    [[nodiscard]] QString lastError() const;

private:
    SqliteDatabase& m_db;
    mutable QString m_lastError;

    [[nodiscard]] bool upsertUnlocked(const FailureHitRow& row);
};

} // namespace storepulse
