find_package(Qt6 6.5 REQUIRED COMPONENTS Test Network)

qt_add_executable(test_db_upserts
    test_db_upserts.cpp
)
target_link_libraries(test_db_upserts
    PRIVATE StorePulse::Db StorePulse::Core Qt6::Test Qt6::Sql
)
add_test(NAME DbUpserts COMMAND test_db_upserts)

qt_add_executable(test_analytics_parsers
    test_analytics_parsers.cpp
)
target_compile_definitions(test_analytics_parsers
    PRIVATE TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(test_analytics_parsers
    PRIVATE StorePulse::Api StorePulse::Core Qt6::Test
)
add_test(NAME AnalyticsParsers COMMAND test_analytics_parsers)

qt_add_executable(test_api_client
    test_api_client.cpp
)
target_link_libraries(test_api_client
    PRIVATE StorePulse::Api StorePulse::Db StorePulse::Core Qt6::Test Qt6::Network Qt6::Sql
)
add_test(NAME ApiClient COMMAND test_api_client)

foreach(t IN ITEMS test_db_upserts test_analytics_parsers test_api_client)
    if(MSVC)
        target_compile_options(${t} PRIVATE /utf-8 /permissive- /W4)
    endif()
endforeach()
