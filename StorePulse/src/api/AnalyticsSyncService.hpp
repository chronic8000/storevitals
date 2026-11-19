#pragma once

#include "AcquisitionRepository.hpp"
#include "AppRepository.hpp"
#include "ExtendedAnalyticsRepository.hpp"
#include "FailureHitRepository.hpp"
#include "InsightsPageviewsClient.hpp"
#include "PartnerCenterClient.hpp"
#include "SqliteDatabase.hpp"

#include <QDate>
#include <QObject>
#include <QPair>
#include <QStringList>
#include <QVector>

namespace storepulse {

/// Full Partner Center analytics pull into SQLite.
class AnalyticsSyncService : public QObject {
    Q_OBJECT
public:
    /// Primary analytics pull window (acquisitions, installs, revenue, funnel, …).
    /// Must cover the longest Dashboard range preset (1 year). Usage is capped separately.
    static constexpr int kRestatementDays = 365;
    static constexpr int kUsageRetentionDays = 90;
    static constexpr int kFailureDetailHashLimit = 20;
    static constexpr int kStackCabLimit = 20;

    AnalyticsSyncService(PartnerCenterClient& client,
                         SqliteDatabase& db,
                         QObject* parent = nullptr);

    void syncApp(const QString& storeId);
    void syncApps(const QStringList& storeIds);
    [[nodiscard]] bool isBusy() const;
    [[nodiscard]] QString lastError() const;

signals:
    void syncProgress(const QString& storeId, const QString& stage, int rows);
    /// stageIndex is 0-based within the current app; stageCount is total pipeline stages.
    void syncStageChanged(const QString& storeId, int stageIndex, int stageCount,
                          const QString& stageName);
    void syncFinished(const QString& storeId, bool ok, const QString& error);

private:
    enum class Stage {
        Funnel,
        ChannelViews,
        InsightsPageviews,
        Acquisitions,
        AppRevenue,
        IapRevenue,
        Installs,
        Usage,
        Ratings,
        Reviews,
        Failures,
        FailureDetails,
        FailureStacks,
        Done
    };

    PartnerCenterClient& m_client;
    InsightsPageviewsClient m_insights;
    SqliteDatabase& m_db;
    AppRepository m_apps;
    AcquisitionRepository m_acquisitions;
    FailureHitRepository m_failures;
    ExtendedAnalyticsRepository m_extended;
    QString m_activeStoreId;
    QStringList m_queue;
    QString m_lastError;
    bool m_busy = false;
    bool m_hadHardFailure = false;
    bool m_clearedStageWindow = false;
    int m_stageRows = 0;
    Stage m_stage = Stage::Done;
    QDate m_windowStart;
    QDate m_windowEnd;
    QDate m_healthStart;
    QStringList m_detailHashQueue;
    QVector<QPair<QString, QString>> m_stackCabQueue; // hash, cabId

    void wireClient();
    void startNextInQueue();
    void beginAppSync(const QString& storeId);
    void runStage(Stage stage);
    void advanceAfterStage(bool ok, const QString& error);
    void fetchNextFailureDetail();
    void fetchNextStackTrace();
    void emitStageProgress();
    [[nodiscard]] static QString stageName(Stage stage);
    [[nodiscard]] static int stageIndex(Stage stage);
    [[nodiscard]] static int stageCount();
};

} // namespace storepulse
