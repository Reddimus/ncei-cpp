add_executable(ncei_tests
    test_error.cpp
    test_rate_limit.cpp
    test_retry.cpp
    test_cache.cpp
    test_date_range.cpp
    test_csv_parser.cpp
    test_pagination.cpp
    test_format.cpp
    test_cdo_models.cpp
    test_cdo_client.cpp
    test_data_service_models.cpp
    test_data_service_client.cpp
)
target_link_libraries(ncei_tests PRIVATE
    ncei_core ncei_http ncei_models ncei_cdo ncei_data
    GTest::gtest_main GTest::gmock
)
target_include_directories(ncei_tests PRIVATE ${json_SOURCE_DIR}/include)
include(GoogleTest)
gtest_discover_tests(ncei_tests)
