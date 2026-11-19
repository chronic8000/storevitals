#pragma once

#include <QSqlDatabase>
#include <QString>

namespace storepulse {

/// Owns the process SQLite connection with WAL + foreign keys enabled.
class SqliteDatabase {
public:
    static constexpr const char* kConnectionName = "storepulse.main";

    SqliteDatabase() = default;
    ~SqliteDatabase();

    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;

    /// Opens (or creates) the DB file, enables WAL, runs migrations.
    [[nodiscard]] bool open(const QString& filePath);
    void close();

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] QSqlDatabase database() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] QString filePath() const;

    /// Begin/commit helpers for repository batch upserts.
    [[nodiscard]] bool beginTransaction();
    [[nodiscard]] bool commit();
    [[nodiscard]] bool rollback();

private:
    QString m_filePath;
    QString m_lastError;
    bool m_opened = false;

    [[nodiscard]] bool configurePragmas();
};

} // namespace storepulse
