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
    test_climate_index.cpp
)
target_link_libraries(ncei_tests PRIVATE
    ncei_core ncei_http ncei_models ncei_cdo ncei_data
    GTest::gtest_main GTest::gmock
)
target_include_directories(ncei_tests PRIVATE ${glaze_SOURCE_DIR}/include)
include(GoogleTest)
gtest_discover_tests(ncei_tests)

if(NOT MSVC)
    target_compile_options(ncei_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Glaze deserializer tests — verifies the migration's parse output matches
# the pre-migration shape (null-safety, missing-field defaults, dynamic
# DataPoint attributes, CDO metadata envelope).
add_executable(ncei_glaze_test
    glaze_test.cpp
)
target_link_libraries(ncei_glaze_test PRIVATE
    gtest_main
    ncei_core ncei_models
)
target_include_directories(ncei_glaze_test PRIVATE ${glaze_SOURCE_DIR}/include)
if(NOT MSVC)
    target_compile_options(ncei_glaze_test PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME ncei_glaze_test COMMAND ncei_glaze_test)

# Glaze parse-throughput regression guard. The bench parses a representative
# CDO /stations list response (metadata envelope + 100 CDOStation results,
# ~21 KB) 1k times and asserts us/op stays under a cap. ctest --timeout = 30s;
# actual wall-clock is ~100ms on x86_64-v3, so the timeout fires only on
# catastrophic regression.
add_executable(ncei_parse_benchmark
    parse_benchmark.cpp
)
target_link_libraries(ncei_parse_benchmark PRIVATE
    ncei_core ncei_models
)
target_include_directories(ncei_parse_benchmark PRIVATE ${glaze_SOURCE_DIR}/include)
if(NOT MSVC)
    target_compile_options(ncei_parse_benchmark PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME ncei_parse_benchmark COMMAND ncei_parse_benchmark)
set_tests_properties(ncei_parse_benchmark PROPERTIES TIMEOUT 30)
