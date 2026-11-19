#pragma once

#include <QSqlDatabase>
#include <QString>

namespace storepulse {

class MigrationRunner {
public:
    explicit MigrationRunner(QSqlDatabase db);

    /// Applies embedded :/storepulse/db/migrations/*.sql in version order.
    [[nodiscard]] bool applyEmbeddedMigrations();
    [[nodiscard]] int currentVersion() const;
    [[nodiscard]] QString lastError() const;

private:
    QSqlDatabase m_db;
    mutable QString m_lastError;

    [[nodiscard]] bool ensureMigrationsTable();
    [[nodiscard]] bool isApplied(int version) const;
    [[nodiscard]] bool recordApplied(int version, const QString& name);
    [[nodiscard]] bool execScript(const QString& sql);
};

} // namespace storepulse
