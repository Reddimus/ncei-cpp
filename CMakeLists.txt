cmake_minimum_required(VERSION 3.31)
project(ncei-cpp VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Options
option(NCEI_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(NCEI_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(NCEI_ENABLE_LTO "Enable Link Time Optimization" ${PROJECT_IS_TOP_LEVEL})
option(NCEI_NATIVE_ARCH "Use -march=native for CPU-specific tuning" OFF)
option(NCEI_ENABLE_NETCDF "Enable NetCDF format support" OFF)

# Enable LTO if requested and supported
if(NCEI_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT LTO_ERROR)
    if(LTO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        message(STATUS "LTO enabled for Release builds")
    else()
        message(WARNING "LTO not supported: ${LTO_ERROR}")
    endif()
endif()

# Native tuning is opt-in; preserve parent release flags.
if(NCEI_NATIVE_ARCH AND NOT MSVC)
    add_compile_options($<$<CONFIG:Release>:-march=native>)
endif()

# Sanitizer support
option(NCEI_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSanitizer" OFF)
if(NCEI_ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
    message(STATUS "Sanitizers enabled (ASan + UBSan)")
endif()

# Code coverage support
option(NCEI_ENABLE_COVERAGE "Enable code coverage instrumentation (gcov)" OFF)
if(NCEI_ENABLE_COVERAGE)
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
    message(STATUS "Code coverage enabled")
endif()

# Find dependencies
find_package(CURL 7.85 REQUIRED)

include(FetchContent)

# Glaze — JSON library (compile-time reflection, 3-4x parse speedup over
# nlohmann on the CDO list-response payload, which is the NCEI hot path).
# License: MIT.
FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG v9.0.0
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(glaze)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add subdirectories
add_subdirectory(src)

if(NCEI_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.18.0
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_subdirectory(tests)
endif()

if(NCEI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install targets
set(NCEI_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/ncei")

install(TARGETS ncei_core ncei_http ncei_models ncei_cdo ncei_data ncei
    EXPORT nceiTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ncei DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/nceiConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/nceiConfig.cmake
    INSTALL_DESTINATION ${NCEI_INSTALL_CMAKEDIR}
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/nceiConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
export(EXPORT nceiTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/nceiTargets.cmake
    NAMESPACE ncei::
)
install(EXPORT nceiTargets
    FILE nceiTargets.cmake
    NAMESPACE ncei::
    DESTINATION ${NCEI_INSTALL_CMAKEDIR}
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/nceiConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/nceiConfigVersion.cmake
    DESTINATION ${NCEI_INSTALL_CMAKEDIR}
)
