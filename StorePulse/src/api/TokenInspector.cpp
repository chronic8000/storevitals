#include "TokenInspector.hpp"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace storepulse {
namespace TokenInspector {
namespace {

QByteArray b64UrlDecode(QString part)
{
    part.replace(QLatin1Char('-'), QLatin1Char('+'));
    part.replace(QLatin1Char('_'), QLatin1Char('/'));
    while (part.size() % 4 != 0) {
        part.append(QLatin1Char('='));
    }
    return QByteArray::fromBase64(part.toUtf8());
}

} // namespace

QString audience(const QString& jwt)
{
    const QStringList parts = jwt.split(QLatin1Char('.'));
    if (parts.size() < 2) {
        return {};
    }
    const QJsonObject payload = QJsonDocument::fromJson(b64UrlDecode(parts.at(1))).object();
    const QJsonValue aud = payload.value(QStringLiteral("aud"));
    if (aud.isString()) {
        return aud.toString();
    }
    if (aud.isArray() && !aud.toArray().isEmpty()) {
        return aud.toArray().at(0).toString();
    }
    return {};
}

bool isDevCenterAudience(const QString& jwt)
{
    const QString aud = audience(jwt);
    return aud.contains(QStringLiteral("manage.devcenter.microsoft.com"), Qt::CaseInsensitive)
           || aud.contains(QStringLiteral("api.partnercenter.microsoft.com"), Qt::CaseInsensitive);
}

} // namespace TokenInspector
} // namespace storepulse
