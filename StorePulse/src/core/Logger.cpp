#include "Logger.hpp"

#include <QDateTime>
#include <mutex>

namespace storepulse {
namespace {

LogLevel g_minLevel = LogLevel::Info;
std::mutex g_logMutex;

const char* levelName(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

} // namespace

void Logger::setMinLevel(LogLevel level)
{
    g_minLevel = level;
}

void Logger::debug(const QString& message) { write(LogLevel::Debug, message); }
void Logger::info(const QString& message) { write(LogLevel::Info, message); }
void Logger::warning(const QString& message) { write(LogLevel::Warning, message); }
void Logger::error(const QString& message) { write(LogLevel::Error, message); }

void Logger::write(LogLevel level, const QString& message)
{
    if (static_cast<int>(level) < static_cast<int>(g_minLevel)) {
        return;
    }

    const QString line = QStringLiteral("[%1] [%2] %3")
                             .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate),
                                  QLatin1String(levelName(level)),
                                  message);

    std::lock_guard lock(g_logMutex);
    switch (level) {
    case LogLevel::Debug:
        qDebug().noquote() << line;
        break;
    case LogLevel::Info:
        qInfo().noquote() << line;
        break;
    case LogLevel::Warning:
        qWarning().noquote() << line;
        break;
    case LogLevel::Error:
        qCritical().noquote() << line;
        break;
    }
}

} // namespace storepulse
