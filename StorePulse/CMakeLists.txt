cmake_minimum_required(VERSION 3.24)

project(StorePulse
    VERSION 1.0.0
    DESCRIPTION "StoreVitals — native Windows Partner Center analytics dashboard"
    LANGUAGES CXX
)

# Also expose at the top level so packaging scripts / StoreLicenseManager see it.
option(DEV_BYPASS_STORE_LICENSE
    "Bypass Windows.Services.Store license checks for unpackaged local builds"
    ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "Store Pulse currently targets Windows only (Credential Manager / WinRT).")
endif()

option(STOREPULSE_BUILD_TESTS "Build unit tests" ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Gui Network Sql Qml Quick QuickControls2
)
qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(src/core)
add_subdirectory(src/db)
add_subdirectory(src/api)
add_subdirectory(src/ui)

qt_add_executable(StorePulse
    src/main.cpp
)
# Product brand is StoreVitals; CMake target stays StorePulse for less churn.
set_target_properties(StorePulse PROPERTIES OUTPUT_NAME "StoreVitals")

qt_add_qml_module(StorePulse
    URI StorePulse
    VERSION 1.0
    QML_FILES
        src/ui/qml/Main.qml
        src/ui/qml/SettingsPage.qml
        src/ui/qml/DashboardPage.qml
        src/ui/qml/ReviewInboxDrawer.qml
    NO_RESOURCE_TARGET_PATH
)

# App icon (Windows .ico) when present.
set(STOREPULSE_ICO "${CMAKE_CURRENT_SOURCE_DIR}/branding/StorePulse.ico")
if(EXISTS "${STOREPULSE_ICO}")
    set(STOREPULSE_RC "${CMAKE_CURRENT_BINARY_DIR}/storepulse_app.rc")
    string(REPLACE "\\" "/" STOREPULSE_ICO_FWD "${STOREPULSE_ICO}")
    file(WRITE "${STOREPULSE_RC}" "IDI_ICON1 ICON DISCARDABLE \"${STOREPULSE_ICO_FWD}\"\n")
    target_sources(StorePulse PRIVATE "${STOREPULSE_RC}")
endif()

target_link_libraries(StorePulse
    PRIVATE
        StorePulse::Core
        StorePulse::Db
        StorePulse::Api
        StorePulse::Ui
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
)

if(MSVC)
    target_compile_options(StorePulse PRIVATE /utf-8 /permissive- /W4)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
    set(STOREPULSE_ARCH "arm64")
else()
    set(STOREPULSE_ARCH "x64")
endif()
message(STATUS "StorePulse target architecture: ${STOREPULSE_ARCH}")

if(STOREPULSE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
