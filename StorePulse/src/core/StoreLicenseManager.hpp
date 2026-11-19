#pragma once

#include <QObject>
#include <QString>

namespace storepulse {

/// Microsoft Store license gate via Windows.Services.Store (C++/WinRT).
/// With -DDEV_BYPASS_STORE_LICENSE=ON, always reports an active full license
/// so unpacked local builds run without a Store package identity.
class StoreLicenseManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready NOTIFY licenseChanged)
    Q_PROPERTY(bool active READ active NOTIFY licenseChanged)
    Q_PROPERTY(bool trial READ trial NOTIFY licenseChanged)
    Q_PROPERTY(bool bypassed READ bypassed CONSTANT)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY licenseChanged)

public:
    explicit StoreLicenseManager(QObject* parent = nullptr);

    [[nodiscard]] bool ready() const { return m_ready; }
    [[nodiscard]] bool active() const { return m_active; }
    [[nodiscard]] bool trial() const { return m_trial; }
    [[nodiscard]] bool bypassed() const;
    [[nodiscard]] QString statusMessage() const { return m_status; }

    /// Bind StoreContext to the main window HWND (required for desktop apps).
    Q_INVOKABLE void initializeWithWindow(qulonglong hwnd);
    Q_INVOKABLE void refresh();

signals:
    void licenseChanged();

private:
    bool m_ready = false;
    bool m_active = false;
    bool m_trial = false;
    QString m_status = QStringLiteral("Checking Store license…");
    qulonglong m_hwnd = 0;

    void applyBypass();
    void queryLicense();
};

} // namespace storepulse
