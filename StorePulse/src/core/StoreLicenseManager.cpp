#include "StoreLicenseManager.hpp"

#include "Logger.hpp"

#ifdef STOREPULSE_DEV_BYPASS_STORE_LICENSE
#define STOREPULSE_LICENSE_BYPASS 1
#else
#define STOREPULSE_LICENSE_BYPASS 0
#endif

#if !STOREPULSE_LICENSE_BYPASS
#include <Windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Services.Store.h>
#include <shobjidl.h>
#endif

namespace storepulse {

StoreLicenseManager::StoreLicenseManager(QObject* parent)
    : QObject(parent)
{
    if (bypassed()) {
        applyBypass();
    }
}

bool StoreLicenseManager::bypassed() const
{
#if STOREPULSE_LICENSE_BYPASS
    return true;
#else
    return false;
#endif
}

void StoreLicenseManager::applyBypass()
{
    m_ready = true;
    m_active = true;
    m_trial = false;
    m_status = QStringLiteral("Dev license bypass (unpackaged build)");
    Logger::info(m_status);
    emit licenseChanged();
}

void StoreLicenseManager::initializeWithWindow(qulonglong hwnd)
{
    m_hwnd = hwnd;
    if (bypassed()) {
        applyBypass();
        return;
    }
    refresh();
}

void StoreLicenseManager::refresh()
{
    if (bypassed()) {
        applyBypass();
        return;
    }
    queryLicense();
}

void StoreLicenseManager::queryLicense()
{
#if STOREPULSE_LICENSE_BYPASS
    applyBypass();
#else
    try {
        static bool apartmentReady = false;
        if (!apartmentReady) {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
            apartmentReady = true;
        }
        auto context = winrt::Windows::Services::Store::StoreContext::GetDefault();
        if (m_hwnd != 0) {
            auto init = context.as<::IInitializeWithWindow>();
            init->Initialize(reinterpret_cast<HWND>(static_cast<uintptr_t>(m_hwnd)));
        }

        auto async = context.GetAppLicenseAsync();
        async.Completed([this](auto const& op, auto const status) {
            QMetaObject::invokeMethod(
                this,
                [this, op, status]() {
                    try {
                        if (status != winrt::Windows::Foundation::AsyncStatus::Completed) {
                            m_ready = true;
                            m_active = false;
                            m_trial = false;
                            m_status = QStringLiteral("Store license check failed");
                            Logger::warning(m_status);
                            emit licenseChanged();
                            return;
                        }
                        const auto license = op.GetResults();
                        m_ready = true;
                        m_active = license.IsActive();
                        m_trial = license.IsTrial();
                        if (!m_active) {
                            m_status = QStringLiteral("No active Store license — purchase required");
                        } else if (m_trial) {
                            m_status = QStringLiteral("Store trial license active");
                        } else {
                            m_status = QStringLiteral("Store license active");
                        }
                        Logger::info(m_status);
                        emit licenseChanged();
                    } catch (const winrt::hresult_error& ex) {
                        m_ready = true;
                        m_active = false;
                        m_status = QStringLiteral("Store license error: %1")
                                       .arg(QString::fromStdWString(ex.message().c_str()));
                        Logger::error(m_status);
                        emit licenseChanged();
                    }
                },
                Qt::QueuedConnection);
        });

        // Offline resilience: refresh when cached licenses change (once).
        static bool offlineHooked = false;
        if (!offlineHooked) {
            offlineHooked = true;
            context.OfflineLicensesChanged([this](auto&&, auto&&) {
                QMetaObject::invokeMethod(this, [this]() { refresh(); }, Qt::QueuedConnection);
            });
        }
    } catch (const winrt::hresult_error& ex) {
        m_ready = true;
        m_active = false;
        m_status = QStringLiteral("Store APIs unavailable: %1")
                       .arg(QString::fromStdWString(ex.message().c_str()));
        Logger::error(m_status);
        emit licenseChanged();
    } catch (...) {
        m_ready = true;
        m_active = false;
        m_status = QStringLiteral("Store license check threw an unknown error");
        Logger::error(m_status);
        emit licenseChanged();
    }
#endif
}

} // namespace storepulse
