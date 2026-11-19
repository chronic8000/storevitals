qt_add_library(StorePulseUi STATIC
    AuthViewModel.cpp
    AppController.cpp
    DashboardViewModel.cpp
)

add_library(StorePulse::Ui ALIAS StorePulseUi)

target_include_directories(StorePulseUi
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StorePulseUi
    PUBLIC
        StorePulse::Core
        StorePulse::Api
        StorePulse::Db
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Network
)

if(MSVC)
    target_compile_options(StorePulseUi PRIVATE /utf-8 /permissive- /W4)
endif()
