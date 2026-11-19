#pragma once

#include "Models.hpp"
#include "SqliteDatabase.hpp"

#include <QVector>

namespace storepulse {

class AppRepository {
public:
    explicit AppRepository(SqliteDatabase& db);

    [[nodiscard]] bool upsert(const AppRow& row);
    [[nodiscard]] bool ensureApp(const QString& storeId, const QString& displayName = {});
    [[nodiscard]] QVector<AppRow> listActive() const;
    [[nodiscard]] QString lastError() const;

private:
    SqliteDatabase& m_db;
    mutable QString m_lastError;
};

} // namespace storepulse
