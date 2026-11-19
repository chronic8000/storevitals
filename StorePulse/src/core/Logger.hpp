#pragma once

#include <QString>
#include <QtLogging>

namespace storepulse {

enum class LogLevel { Debug, Info, Warning, Error };

/// Minimal logger. Never logs secrets, tokens, or PII (spec §2).
class Logger {
public:
    static void setMinLevel(LogLevel level);
    static void debug(const QString& message);
    static void info(const QString& message);
    static void warning(const QString& message);
    static void error(const QString& message);

private:
    static void write(LogLevel level, const QString& message);
};

} // namespace storepulse
