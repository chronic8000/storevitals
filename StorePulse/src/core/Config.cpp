#include "Config.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace storepulse {
namespace {

constexpr auto kOrg = "StoreVitals";
constexpr auto kApp = "StoreVitals";
constexpr auto kAuthMode = "auth/mode";
constexpr auto kAudience = "auth/audience";
constexpr auto kInteractiveClientId = "auth/interactiveClientId";
constexpr auto kTenantId = "auth/tenantId";
constexpr auto kClientId = "auth/clientId";
constexpr auto kSecretExpires = "auth/clientSecretExpiresOn";
constexpr auto kWarningDays = "auth/secretExpiryWarningDays";
constexpr auto kDbPath = "db/path";

QString defaultDatabasePath()
{
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(root);
    const QString neu = QDir(root).filePath(QStringLiteral("storevitals.sqlite"));
    if (QFileInfo::exists(neu)) {
        return neu;
    }
    // One-time carry-over from the pre-rename Store Pulse AppData folder.
    const QString legacy = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
                               .filePath(QStringLiteral("StorePulse/StorePulse/storepulse.sqlite"));
    if (QFileInfo::exists(legacy) && QFile::copy(legacy, neu)) {
        return neu;
    }
    return neu;
}

} // namespace

Config& Config::instance()
{
    static Config config;
    return config;
}

Config::Config()
    : m_settings(QSettings::IniFormat, QSettings::UserScope, QLatin1String(kOrg), QLatin1String(kApp))
{
    // Import Store Pulse settings when StoreVitals auth fields are still empty.
    const bool needsLegacyAuth =
        m_settings.value(QLatin1String(kInteractiveClientId)).toString().trimmed().isEmpty()
        && m_settings.value(QLatin1String(kTenantId)).toString().trimmed().isEmpty()
        && m_settings.value(QLatin1String(kClientId)).toString().trimmed().isEmpty();
    if (needsLegacyAuth) {
        QSettings legacy(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("StorePulse"),
                         QStringLiteral("StorePulse"));
        const QStringList keys = {
            QLatin1String(kAuthMode),
            QLatin1String(kAudience),
            QLatin1String(kInteractiveClientId),
            QLatin1String(kTenantId),
            QLatin1String(kClientId),
            QLatin1String(kSecretExpires),
            QLatin1String(kWarningDays),
        };
        for (const QString& key : keys) {
            if (!legacy.contains(key)) {
                continue;
            }
            const QVariant legacyValue = legacy.value(key);
            if (legacyValue.toString().trimmed().isEmpty() && !legacyValue.toDate().isValid()) {
                continue;
            }
            m_settings.setValue(key, legacyValue);
        }
        m_settings.sync();
    }

    if (!m_settings.contains(QLatin1String(kDbPath))) {
        setDatabasePath(defaultDatabasePath());
    }
    if (!m_settings.contains(QLatin1String(kWarningDays))) {
        setSecretExpiryWarningDays(14);
    }
    if (!m_settings.contains(QLatin1String(kAuthMode))) {
        setAuthMode(QStringLiteral("interactive"));
    }
    if (!m_settings.contains(QLatin1String(kAudience))) {
        setAccountAudience(QStringLiteral("common"));
    }
}

QString Config::authMode() const
{
    return m_settings.value(QLatin1String(kAuthMode), QStringLiteral("interactive")).toString();
}

void Config::setAuthMode(const QString& mode)
{
    m_settings.setValue(QLatin1String(kAuthMode), mode);
}

QString Config::accountAudience() const
{
    return m_settings.value(QLatin1String(kAudience), QStringLiteral("common")).toString();
}

void Config::setAccountAudience(const QString& audience)
{
    m_settings.setValue(QLatin1String(kAudience), audience);
}

QString Config::interactiveClientId() const
{
    return m_settings.value(QLatin1String(kInteractiveClientId)).toString();
}

void Config::setInteractiveClientId(const QString& clientId)
{
    m_settings.setValue(QLatin1String(kInteractiveClientId), clientId);
}

QString Config::tenantId() const
{
    return m_settings.value(QLatin1String(kTenantId)).toString();
}

void Config::setTenantId(const QString& tenantId)
{
    m_settings.setValue(QLatin1String(kTenantId), tenantId);
}

QString Config::clientId() const
{
    return m_settings.value(QLatin1String(kClientId)).toString();
}

void Config::setClientId(const QString& clientId)
{
    m_settings.setValue(QLatin1String(kClientId), clientId);
}

std::optional<QDate> Config::clientSecretExpiresOn() const
{
    const QString raw = m_settings.value(QLatin1String(kSecretExpires)).toString();
    if (raw.isEmpty()) {
        return std::nullopt;
    }
    const QDate date = QDate::fromString(raw, Qt::ISODate);
    if (!date.isValid()) {
        return std::nullopt;
    }
    return date;
}

void Config::setClientSecretExpiresOn(const std::optional<QDate>& date)
{
    if (!date.has_value()) {
        m_settings.remove(QLatin1String(kSecretExpires));
        return;
    }
    m_settings.setValue(QLatin1String(kSecretExpires), date->toString(Qt::ISODate));
}

int Config::secretExpiryWarningDays() const
{
    return m_settings.value(QLatin1String(kWarningDays), 14).toInt();
}

void Config::setSecretExpiryWarningDays(int days)
{
    m_settings.setValue(QLatin1String(kWarningDays), days);
}

bool Config::isSecretExpiringSoon() const
{
    const auto expires = clientSecretExpiresOn();
    if (!expires) {
        return false;
    }
    const int remaining = QDate::currentDate().daysTo(*expires);
    return remaining >= 0 && remaining <= secretExpiryWarningDays();
}

bool Config::isSecretExpired() const
{
    const auto expires = clientSecretExpiresOn();
    if (!expires) {
        return false;
    }
    return QDate::currentDate() > *expires;
}

QString Config::databasePath() const
{
    return m_settings.value(QLatin1String(kDbPath), defaultDatabasePath()).toString();
}

void Config::setDatabasePath(const QString& path)
{
    m_settings.setValue(QLatin1String(kDbPath), path);
}

void Config::sync()
{
    m_settings.sync();
}

} // namespace storepulse
