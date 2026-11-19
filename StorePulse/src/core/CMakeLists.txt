# Prefer the root-level option when available; keep a local default for standalone configure.
if(NOT DEFINED DEV_BYPASS_STORE_LICENSE)
    option(DEV_BYPASS_STORE_LICENSE
        "Bypass Windows.Services.Store license checks for unpackaged local builds"
        ON)
endif()

add_library(StorePulseCore STATIC
    Config.cpp
    CredentialManager.cpp
    Logger.cpp
    StoreLicenseManager.cpp
)

add_library(StorePulse::Core ALIAS StorePulseCore)

target_include_directories(StorePulseCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StorePulseCore
    PUBLIC
        Qt6::Core
    PRIVATE
        Advapi32
)

if(DEV_BYPASS_STORE_LICENSE)
    target_compile_definitions(StorePulseCore PUBLIC STOREPULSE_DEV_BYPASS_STORE_LICENSE=1)
    message(STATUS "Store license: DEV bypass ON (unpackaged builds)")
else()
    target_link_libraries(StorePulseCore PRIVATE windowsapp)
    message(STATUS "Store license: Windows.Services.Store enforcement ON")
endif()

if(MSVC)
    target_compile_options(StorePulseCore PRIVATE /utf-8 /permissive- /W4)
    if(NOT DEV_BYPASS_STORE_LICENSE)
        # C++/WinRT + C++20: do not pass legacy /await (removed on recent MSVC).
        target_compile_definitions(StorePulseCore PRIVATE
            _SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNING=1)
    endif()
endif()
