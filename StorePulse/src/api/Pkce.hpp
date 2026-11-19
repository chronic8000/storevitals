#pragma once

#include <QByteArray>
#include <QString>

namespace storepulse::pkce {

/// RFC 7636 code_verifier (43–128 chars from unreserved set).
[[nodiscard]] QString generateCodeVerifier();

/// BASE64URL(SHA256(verifier)) without padding.
[[nodiscard]] QString challengeS256(const QString& verifier);

[[nodiscard]] QString generateState();

[[nodiscard]] QByteArray sha256(const QByteArray& data);
[[nodiscard]] QString base64UrlEncode(const QByteArray& data);

} // namespace storepulse::pkce
