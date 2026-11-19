add_library(StorePulseDb STATIC
    SqliteDatabase.cpp
    MigrationRunner.cpp
    AcquisitionRepository.cpp
    FailureHitRepository.cpp
    AppRepository.cpp
    DashboardQuery.cpp
    ExtendedAnalyticsRepository.cpp
)

add_library(StorePulse::Db ALIAS StorePulseDb)

target_include_directories(StorePulseDb
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StorePulseDb
    PUBLIC
        StorePulse::Core
        Qt6::Core
        Qt6::Sql
)

# Embed SQL migrations as Qt resources.
qt_add_resources(StorePulseDb "db_migrations"
    PREFIX "/storepulse/db"
    FILES
        migrations/001_initial.sql
        migrations/002_analytics_dimensions.sql
        migrations/003_partner_center_full.sql
        migrations/004_channel_pageviews.sql
        migrations/005_insights_pageviews.sql
        migrations/006_revenue.sql
        migrations/007_health_reviews_channels.sql
)

if(MSVC)
    target_compile_options(StorePulseDb PRIVATE /utf-8 /permissive- /W4)
endif()
