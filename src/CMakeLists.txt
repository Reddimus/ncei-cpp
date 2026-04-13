# Core library (error, rate limiting, retry, date_range, csv_parser)
add_library(ncei_core STATIC
    core/error.cpp
    core/rate_limit.cpp
    core/retry.cpp
    core/date_range.cpp
    core/csv_parser.cpp
    core/pagination.cpp
)
target_compile_features(ncei_core PUBLIC cxx_std_23)
target_include_directories(ncei_core PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${json_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# HTTP client library
add_library(ncei_http STATIC
    http/client.cpp
)
target_link_libraries(ncei_http PUBLIC ncei_core CURL::libcurl)
target_include_directories(ncei_http PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Models library (CDO + Data Service)
add_library(ncei_models STATIC
    models/common.cpp
    models/cdo/dataset.cpp
    models/cdo/data_category.cpp
    models/cdo/data_type.cpp
    models/cdo/location_category.cpp
    models/cdo/location.cpp
    models/cdo/station.cpp
    models/cdo/data.cpp
    models/data_service/data_point.cpp
    models/data_service/search_result.cpp
    models/data_service/dataset_metadata.cpp
)
target_link_libraries(ncei_models PUBLIC ncei_core)
target_include_directories(ncei_models PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# CDO Client library
add_library(ncei_cdo STATIC
    api/cdo_client.cpp
)
target_link_libraries(ncei_cdo PUBLIC ncei_core ncei_http ncei_models)
target_include_directories(ncei_cdo PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Data Service Client library
add_library(ncei_data STATIC
    api/data_service_client.cpp
)
target_link_libraries(ncei_data PUBLIC ncei_core ncei_http ncei_models)
target_include_directories(ncei_data PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Main SDK library (combines all)
add_library(ncei INTERFACE)
target_link_libraries(ncei INTERFACE ncei_core ncei_http ncei_models ncei_cdo ncei_data)
