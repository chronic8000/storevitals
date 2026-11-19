#pragma once

#include <QString>
#include <optional>

namespace storepulse {

/// Windows Credential Manager wrapper (CredWrite / CredRead / CredDelete).
/// Holds the Entra client secret only — never persisted to SQLite or config files.
class CredentialManager {
public:
    static constexpr const char* kDefaultTargetName = "StoreVitals/EntraClientSecret";

    explicit CredentialManager(QString targetName = QString::fromUtf8(kDefaultTargetName));

    [[nodiscard]] bool storeSecret(const QString& secret) const;
    [[nodiscard]] std::optional<QString> readSecret() const;
    [[nodiscard]] bool deleteSecret() const;
    [[nodiscard]] bool hasSecret() const;

    [[nodiscard]] QString lastError() const;
    [[nodiscard]] QString targetName() const;

private:
    QString m_targetName;
    mutable QString m_lastError;

    void setWin32Error(unsigned long code, const QString& context) const;
};

} // namespace storepulse
