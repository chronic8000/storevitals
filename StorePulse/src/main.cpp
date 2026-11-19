#include "AppController.hpp"
#include "Config.hpp"
#include "CredentialManager.hpp"
#include "EntraAuthClient.hpp"
#include "InteractiveAuthClient.hpp"
#include "Logger.hpp"
#include "PartnerCenterClient.hpp"
#include "AnalyticsSyncService.hpp"
#include "SqliteDatabase.hpp"

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QNetworkAccessManager>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <memory>

namespace {

storepulse::AccountAudience parseAudience(const QString& value)
{
    if (value == QLatin1String("consumers")) {
        return storepulse::AccountAudience::Consumers;
    }
    if (value == QLatin1String("organizations")) {
        return storepulse::AccountAudience::Organizations;
    }
    return storepulse::AccountAudience::Common;
}

int runCliSignIn(QGuiApplication& app,
                 storepulse::InteractiveAuthClient& interactive,
                 const QStringList& args)
{
    if (args.contains(QStringLiteral("--device-code"))) {
        interactive.setPreferredFlow(storepulse::InteractiveAuthClient::Flow::DeviceCode);
    }
    QObject::connect(&interactive, &storepulse::AuthProvider::tokenReady, &app,
                     [&app](const QString&, const QDateTime&) {
                         storepulse::Logger::info(QStringLiteral("Sign-in succeeded"));
                         app.quit();
                     });
    QObject::connect(&interactive, &storepulse::AuthProvider::tokenFailed, &app,
                     [&app](const QString& error) {
                         storepulse::Logger::error(error);
                         app.exit(2);
                     });
    QObject::connect(&interactive, &storepulse::AuthProvider::deviceCodeReceived, &app,
                     [](const QString& userCode, const QString& url) {
                         storepulse::Logger::info(
                             QStringLiteral("Open %1 and enter code: %2").arg(url, userCode));
                     });
    interactive.signInInteractively();
    return app.exec();
}

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("StoreVitals"));
    QGuiApplication::setApplicationName(QStringLiteral("StoreVitals"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("StoreVitals"));
    QGuiApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    storepulse::Logger::info(QStringLiteral("StoreVitals %1 starting")
                                 .arg(QCoreApplication::applicationVersion()));

    auto& config = storepulse::Config::instance();
    const QString dbPath = config.databasePath();
    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    storepulse::SqliteDatabase db;
    if (!db.open(dbPath)) {
        storepulse::Logger::error(QStringLiteral("Database bootstrap failed: %1").arg(db.lastError()));
        return 1;
    }

    QNetworkAccessManager nam;
    const QStringList args = QCoreApplication::arguments();
    const bool cliSignIn = args.contains(QStringLiteral("--signin"));
    const bool cliSync = args.contains(QStringLiteral("--sync"));

    // Headless CLI paths keep using a dedicated auth object.
    if (cliSignIn || cliSync) {
        std::unique_ptr<storepulse::AuthProvider> auth;
        storepulse::InteractiveAuthClient* interactivePtr = nullptr;

        if (cliSignIn) {
            auto interactive = std::make_unique<storepulse::InteractiveAuthClient>(&nam);
            interactive->setClientId(config.interactiveClientId());
            interactive->setAudience(parseAudience(config.accountAudience()));
            interactivePtr = interactive.get();
            auth = std::move(interactive);
            return runCliSignIn(app, *interactivePtr, args);
        }

        // --sync always uses Dev Center client-credentials (analytics requirement).
        storepulse::CredentialManager creds;
        if (config.tenantId().trimmed().isEmpty() || config.clientId().trimmed().isEmpty()
            || !creds.hasSecret()) {
            storepulse::Logger::error(
                QStringLiteral("--sync needs tenant ID, client ID, and client secret in Settings"));
            return 1;
        }
        auto entra = std::make_unique<storepulse::EntraAuthClient>(&nam);
        entra->setTenantId(config.tenantId());
        entra->setClientId(config.clientId());
        auth = std::move(entra);

        storepulse::PartnerCenterClient client(*auth, &nam);
        storepulse::AnalyticsSyncService sync(client, db);
        QString storeId = QStringLiteral("9P0HGKHQ9W19");
        const int idx = args.indexOf(QStringLiteral("--sync"));
        if (idx + 1 < args.size() && !args.at(idx + 1).startsWith(QLatin1Char('-'))) {
            storeId = args.at(idx + 1);
        }
        QObject::connect(&sync, &storepulse::AnalyticsSyncService::syncFinished, &app,
                         [&app](const QString&, bool ok, const QString& error) {
                             if (!ok) {
                                 storepulse::Logger::error(error);
                                 app.exit(2);
                             } else {
                                 app.quit();
                             }
                         });
        sync.syncApp(storeId);
        return app.exec();
    }

    storepulse::AppController controller(db, &nam);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule(u"StorePulse", u"Main");

    if (engine.rootObjects().isEmpty()) {
        storepulse::Logger::error(QStringLiteral("Failed to load QML UI"));
        return 1;
    }

    // Bind StoreContext to the main window HWND after the Quick window exists.
    QTimer::singleShot(0, &app, [&engine, &controller]() {
        if (engine.rootObjects().isEmpty()) {
            return;
        }
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
        if (!window) {
            return;
        }
        const qulonglong hwnd = static_cast<qulonglong>(window->winId());
        controller.license()->initializeWithWindow(hwnd);
    });

    return app.exec();
}
