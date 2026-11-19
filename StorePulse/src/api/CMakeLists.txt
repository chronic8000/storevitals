add_library(StorePulseApi STATIC
    AuthProvider.cpp
    Pkce.cpp
    TokenInspector.cpp
    EntraAuthClient.cpp
    InteractiveAuthClient.cpp
    PartnerCenterClient.cpp
    InsightsPageviewsClient.cpp
    AnalyticsParsers.cpp
    RateLimiter.cpp
    AnalyticsSyncService.cpp
)

add_library(StorePulse::Api ALIAS StorePulseApi)

target_include_directories(StorePulseApi
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StorePulseApi
    PUBLIC
        StorePulse::Core
        StorePulse::Db
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Sql
)

if(MSVC)
    target_compile_options(StorePulseApi PRIVATE /utf-8 /permissive- /W4)
endif()
