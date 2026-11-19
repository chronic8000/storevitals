#include "CredentialManager.hpp"

#include "Logger.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <wincred.h>

#include <vector>

namespace storepulse {

CredentialManager::CredentialManager(QString targetName)
    : m_targetName(std::move(targetName))
{
}

QString CredentialManager::targetName() const
{
    return m_targetName;
}

QString CredentialManager::lastError() const
{
    return m_lastError;
}

void CredentialManager::setWin32Error(unsigned long code, const QString& context) const
{
    wchar_t* buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                        | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD len = FormatMessageW(flags, nullptr, code, 0,
                                     reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);

    QString systemMessage;
    if (len > 0 && buffer != nullptr) {
        systemMessage = QString::fromWCharArray(buffer, static_cast<int>(len)).trimmed();
        LocalFree(buffer);
    } else {
        systemMessage = QStringLiteral("Unknown error");
    }

    m_lastError = QStringLiteral("%1 failed (0x%2): %3")
                      .arg(context)
                      .arg(code, 8, 16, QLatin1Char('0'))
                      .arg(systemMessage);

    // Never echo the secret; only the Win32 context.
    Logger::error(m_lastError);
}

bool CredentialManager::storeSecret(const QString& secret) const
{
    m_lastError.clear();

    const QString trimmed = secret.trimmed();
    if (trimmed.isEmpty()) {
        m_lastError = QStringLiteral("Refusing to store an empty client secret");
        Logger::error(m_lastError);
        return false;
    }

    const std::wstring target = m_targetName.toStdWString();
    // CredentialBlob is a byte length of UTF-16 code units (not including null).
    std::wstring blob = trimmed.toStdWString();

    CREDENTIALW cred{};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = const_cast<LPWSTR>(target.c_str());
    cred.CredentialBlobSize = static_cast<DWORD>(blob.size() * sizeof(wchar_t));
    cred.CredentialBlob = reinterpret_cast<LPBYTE>(blob.data());
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = const_cast<LPWSTR>(L"StoreVitals");

    if (!CredWriteW(&cred, 0)) {
        setWin32Error(GetLastError(), QStringLiteral("CredWriteW"));
        // Securely wipe local copy.
        SecureZeroMemory(blob.data(), blob.size() * sizeof(wchar_t));
        return false;
    }

    SecureZeroMemory(blob.data(), blob.size() * sizeof(wchar_t));
    Logger::info(QStringLiteral("Client secret stored in Windows Credential Manager"));
    return true;
}

std::optional<QString> CredentialManager::readSecret() const
{
    m_lastError.clear();

    const std::wstring target = m_targetName.toStdWString();
    PCREDENTIALW cred = nullptr;

    if (!CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &cred)) {
        const DWORD err = GetLastError();
        if (err == ERROR_NOT_FOUND) {
            // Pre-rename fallback (Store Pulse → StoreVitals).
            if (m_targetName == QLatin1String(kDefaultTargetName)
                || m_targetName.endsWith(QLatin1String("/EntraClientSecret"))) {
                const std::wstring legacy = L"StorePulse/EntraClientSecret";
                if (CredReadW(legacy.c_str(), CRED_TYPE_GENERIC, 0, &cred)) {
                    // fall through to blob parse below
                } else {
                    m_lastError = QStringLiteral("No client secret found in Credential Manager");
                    return std::nullopt;
                }
            } else if (m_targetName.endsWith(QLatin1String("/MicrosoftRefreshToken"))) {
                const std::wstring legacy = L"StorePulse/MicrosoftRefreshToken";
                if (CredReadW(legacy.c_str(), CRED_TYPE_GENERIC, 0, &cred)) {
                    // fall through
                } else {
                    m_lastError = QStringLiteral("No client secret found in Credential Manager");
                    return std::nullopt;
                }
            } else {
                m_lastError = QStringLiteral("No client secret found in Credential Manager");
                return std::nullopt;
            }
        } else {
            setWin32Error(err, QStringLiteral("CredReadW"));
            return std::nullopt;
        }
    }

    QString secret;
    if (cred->CredentialBlob != nullptr && cred->CredentialBlobSize > 0) {
        const auto charCount = static_cast<int>(cred->CredentialBlobSize / sizeof(wchar_t));
        secret = QString::fromWCharArray(reinterpret_cast<const wchar_t*>(cred->CredentialBlob),
                                         charCount);
    }

    CredFree(cred);

    if (secret.isEmpty()) {
        m_lastError = QStringLiteral("Credential blob was empty");
        return std::nullopt;
    }
    return secret;
}

bool CredentialManager::deleteSecret() const
{
    m_lastError.clear();
    const std::wstring target = m_targetName.toStdWString();

    if (!CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0)) {
        const DWORD err = GetLastError();
        if (err == ERROR_NOT_FOUND) {
            return true;
        }
        setWin32Error(err, QStringLiteral("CredDeleteW"));
        return false;
    }

    Logger::info(QStringLiteral("Client secret removed from Windows Credential Manager"));
    return true;
}

bool CredentialManager::hasSecret() const
{
    const auto secret = readSecret();
    if (!secret) {
        return false;
    }
    // Wipe by letting QString go out of scope; avoid logging.
    return !secret->isEmpty();
}

} // namespace storepulse
