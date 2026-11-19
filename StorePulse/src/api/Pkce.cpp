#include "Pkce.hpp"

#include <QCryptographicHash>
#include <QRandomGenerator>

namespace storepulse::pkce {
namespace {

QString randomUnreserved(int length)
{
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    QString out;
    out.resize(length);
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < length; ++i) {
        out[i] = QLatin1Char(alphabet[rng->bounded(static_cast<int>(sizeof(alphabet) - 1))]);
    }
    return out;
}

} // namespace

QByteArray sha256(const QByteArray& data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QString base64UrlEncode(const QByteArray& data)
{
    return QString::fromLatin1(data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QString generateCodeVerifier()
{
    return randomUnreserved(64);
}

QString challengeS256(const QString& verifier)
{
    return base64UrlEncode(sha256(verifier.toUtf8()));
}

QString generateState()
{
    return randomUnreserved(32);
}

} // namespace storepulse::pkce
