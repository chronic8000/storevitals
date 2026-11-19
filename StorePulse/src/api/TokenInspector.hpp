#pragma once

#include <QString>

namespace storepulse {

/// Lightweight JWT helpers (no signature verification — local UX only).
namespace TokenInspector {

[[nodiscard]] QString audience(const QString& jwt);
[[nodiscard]] bool isDevCenterAudience(const QString& jwt);

} // namespace TokenInspector
} // namespace storepulse
