#pragma once

#include "AppRepository.hpp"
#include "DashboardQuery.hpp"
#include "SqliteDatabase.hpp"

#include <QDate>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace storepulse {

class DashboardViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList apps READ apps NOTIFY dataChanged)
    Q_PROPERTY(QString selectedStoreId READ selectedStoreId WRITE setSelectedStoreId NOTIFY selectedStoreIdChanged)
    Q_PROPERTY(QString selectedDisplayName READ selectedDisplayName NOTIFY dataChanged)
    Q_PROPERTY(int windowDays READ windowDays WRITE setWindowDays NOTIFY windowDaysChanged)
    Q_PROPERTY(bool customRange READ customRange WRITE setCustomRange NOTIFY rangeChanged)
    Q_PROPERTY(QString customFrom READ customFrom WRITE setCustomFrom NOTIFY rangeChanged)
    Q_PROPERTY(QString customTo READ customTo WRITE setCustomTo NOTIFY rangeChanged)
    Q_PROPERTY(QString rangeLabel READ rangeLabel NOTIFY rangeChanged)
    Q_PROPERTY(QVariantMap kpis READ kpis NOTIFY dataChanged)
    Q_PROPERTY(QVariantList dailySeries READ dailySeries NOTIFY dataChanged)
    Q_PROPERTY(QVariantList markets READ markets NOTIFY dataChanged)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY dataChanged)
    Q_PROPERTY(QVariantList osVersions READ osVersions NOTIFY dataChanged)
    Q_PROPERTY(QVariantList packageVersions READ packageVersions NOTIFY dataChanged)
    Q_PROPERTY(QVariantList versionHealth READ versionHealth NOTIFY dataChanged)
    Q_PROPERTY(QVariantList architectures READ architectures NOTIFY dataChanged)
    Q_PROPERTY(QVariantList acquisitionTypes READ acquisitionTypes NOTIFY dataChanged)
    Q_PROPERTY(QVariantList storeClients READ storeClients NOTIFY dataChanged)
    Q_PROPERTY(QVariantList ageGroups READ ageGroups NOTIFY dataChanged)
    Q_PROPERTY(QVariantList genders READ genders NOTIFY dataChanged)
    Q_PROPERTY(QVariantList channels READ channels NOTIFY dataChanged)
    Q_PROPERTY(QVariantList referrers READ referrers NOTIFY dataChanged)
    Q_PROPERTY(QVariantList campaigns READ campaigns NOTIFY dataChanged)
    Q_PROPERTY(QVariantList ratingStars READ ratingStars NOTIFY dataChanged)
    Q_PROPERTY(QVariantList revenueProducts READ revenueProducts NOTIFY dataChanged)
    Q_PROPERTY(QVariantList reviews READ reviews NOTIFY dataChanged)
    Q_PROPERTY(QVariantList topFailures READ topFailures NOTIFY dataChanged)
    Q_PROPERTY(QVariantList failureDevices READ failureDevices NOTIFY dataChanged)
    Q_PROPERTY(QVariantList failureCpus READ failureCpus NOTIFY dataChanged)
    Q_PROPERTY(QVariantList failureInsider READ failureInsider NOTIFY dataChanged)
    Q_PROPERTY(QVariantList failureCabs READ failureCabs NOTIFY dataChanged)
    Q_PROPERTY(bool lowRamReviewsOnly READ lowRamReviewsOnly WRITE setLowRamReviewsOnly NOTIFY lowRamReviewsOnlyChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit DashboardViewModel(SqliteDatabase& db, QObject* parent = nullptr);

    [[nodiscard]] QVariantList apps() const { return m_apps; }
    [[nodiscard]] QString selectedStoreId() const { return m_selectedStoreId; }
    [[nodiscard]] QString selectedDisplayName() const;
    [[nodiscard]] int windowDays() const { return m_windowDays; }
    [[nodiscard]] bool customRange() const { return m_customRange; }
    [[nodiscard]] QString customFrom() const { return m_customFrom; }
    [[nodiscard]] QString customTo() const { return m_customTo; }
    [[nodiscard]] QString rangeLabel() const;
    [[nodiscard]] QVariantMap kpis() const { return m_kpis; }
    [[nodiscard]] QVariantList dailySeries() const { return m_daily; }
    [[nodiscard]] QVariantList markets() const { return m_markets; }
    [[nodiscard]] QVariantList devices() const { return m_devices; }
    [[nodiscard]] QVariantList osVersions() const { return m_osVersions; }
    [[nodiscard]] QVariantList packageVersions() const { return m_packages; }
    [[nodiscard]] QVariantList versionHealth() const { return m_versionHealth; }
    [[nodiscard]] QVariantList architectures() const { return m_architectures; }
    [[nodiscard]] QVariantList acquisitionTypes() const { return m_acquisitionTypes; }
    [[nodiscard]] QVariantList storeClients() const { return m_storeClients; }
    [[nodiscard]] QVariantList ageGroups() const { return m_ageGroups; }
    [[nodiscard]] QVariantList genders() const { return m_genders; }
    [[nodiscard]] QVariantList channels() const { return m_channels; }
    [[nodiscard]] QVariantList referrers() const { return m_referrers; }
    [[nodiscard]] QVariantList campaigns() const { return m_campaigns; }
    [[nodiscard]] QVariantList ratingStars() const { return m_ratingStars; }
    [[nodiscard]] QVariantList revenueProducts() const { return m_revenueProducts; }
    [[nodiscard]] QVariantList reviews() const { return m_reviews; }
    [[nodiscard]] QVariantList topFailures() const { return m_topFailures; }
    [[nodiscard]] QVariantList failureDevices() const { return m_failureDevices; }
    [[nodiscard]] QVariantList failureCpus() const { return m_failureCpus; }
    [[nodiscard]] QVariantList failureInsider() const { return m_failureInsider; }
    [[nodiscard]] QVariantList failureCabs() const { return m_failureCabs; }
    [[nodiscard]] bool lowRamReviewsOnly() const { return m_lowRamReviewsOnly; }
    [[nodiscard]] QString status() const { return m_status; }

    void setSelectedStoreId(const QString& storeId);
    void setWindowDays(int days);
    void setCustomRange(bool enabled);
    void setCustomFrom(const QString& isoDate);
    void setCustomTo(const QString& isoDate);
    void setLowRamReviewsOnly(bool enabled);

    Q_INVOKABLE void applyCustomRange(const QString& fromIso, const QString& toIso);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool addApp(const QString& storeId, const QString& displayName);
    Q_INVOKABLE QStringList activeStoreIds() const;
    Q_INVOKABLE QString exportAcquisitionsCsv(const QString& filePath);
    Q_INVOKABLE QString exportInstallsCsv(const QString& filePath);
    Q_INVOKABLE QString exportFailuresCsv(const QString& filePath);
    Q_INVOKABLE QString defaultExportPath(const QString& kind) const;

signals:
    void dataChanged();
    void selectedStoreIdChanged();
    void windowDaysChanged();
    void rangeChanged();
    void lowRamReviewsOnlyChanged();
    void statusChanged();

private:
    SqliteDatabase& m_db;
    AppRepository m_appsRepo;
    DashboardQuery m_query;
    QString m_selectedStoreId;
    int m_windowDays = 90;
    bool m_customRange = false;
    QString m_customFrom;
    QString m_customTo;
    QVariantList m_apps;
    QVariantMap m_kpis;
    QVariantList m_daily;
    QVariantList m_markets;
    QVariantList m_devices;
    QVariantList m_osVersions;
    QVariantList m_packages;
    QVariantList m_versionHealth;
    QVariantList m_architectures;
    QVariantList m_acquisitionTypes;
    QVariantList m_storeClients;
    QVariantList m_ageGroups;
    QVariantList m_genders;
    QVariantList m_channels;
    QVariantList m_referrers;
    QVariantList m_campaigns;
    QVariantList m_ratingStars;
    QVariantList m_revenueProducts;
    QVariantList m_reviews;
    QVariantList m_topFailures;
    QVariantList m_failureDevices;
    QVariantList m_failureCpus;
    QVariantList m_failureInsider;
    QVariantList m_failureCabs;
    bool m_lowRamReviewsOnly = false;
    QString m_status;

    void setStatus(const QString& status);
    [[nodiscard]] QPair<QDate, QDate> activeWindow() const;
    [[nodiscard]] static QString normalizePath(const QString& filePath);
    [[nodiscard]] bool writeCsv(const QString& path, const QString& csv);
};

} // namespace storepulse
