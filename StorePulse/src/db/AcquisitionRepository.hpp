#pragma once

#include "Models.hpp"
#include "SqliteDatabase.hpp"

#include <QDate>
#include <QVector>

namespace storepulse {

/// Upsert acquisitions; supports 10-day restatement reconcile.
class AcquisitionRepository {
public:
    explicit AcquisitionRepository(SqliteDatabase& db);

    /// Insert or replace quantity for the natural key; sets restated_at when updating.
    [[nodiscard]] bool upsert(const AcquisitionRow& row);
    [[nodiscard]] bool upsertBatch(const QVector<AcquisitionRow>& rows);

    /// Drop legacy aggregate-only rows (empty dimensions) before dimensional restatement.
    [[nodiscard]] bool clearUndimensionedWindow(const QString& storeId,
                                                const QDate& fromUtc,
                                                const QDate& toUtc);

    /// Replace strategy: wipe the window before dimensional upsert.
    [[nodiscard]] bool clearWindow(const QString& storeId,
                                   const QDate& fromUtc,
                                   const QDate& toUtc);

    /// Re-apply a restatement window (typically last 10 days inclusive).
    [[nodiscard]] bool upsertRestatementWindow(const QString& storeId,
                                               const QDate& fromUtc,
                                               const QDate& toUtc,
                                               const QVector<AcquisitionRow>& rows);

    [[nodiscard]] qint64 sumQuantity(const QString& storeId,
                                     const QDate& fromUtc,
                                     const QDate& toUtc) const;

    [[nodiscard]] QString lastError() const;

private:
    SqliteDatabase& m_db;
    mutable QString m_lastError;

    [[nodiscard]] bool upsertUnlocked(const AcquisitionRow& row);
};

} // namespace storepulse
