# CDO examples
add_executable(basic_cdo_usage basic_cdo_usage.cpp)
target_link_libraries(basic_cdo_usage PRIVATE ncei)

add_executable(historical_temperature historical_temperature.cpp)
target_link_libraries(historical_temperature PRIVATE ncei)

add_executable(multi_year_data multi_year_data.cpp)
target_link_libraries(multi_year_data PRIVATE ncei)

# Data Service examples
add_executable(format_comparison format_comparison.cpp)
target_link_libraries(format_comparison PRIVATE ncei)

add_executable(data_service_search data_service_search.cpp)
target_link_libraries(data_service_search PRIVATE ncei)
