cmake_minimum_required(VERSION 3.20)
project(ncei-cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Options
option(NCEI_BUILD_TESTS "Build tests" ON)
option(NCEI_BUILD_EXAMPLES "Build examples" ON)
option(NCEI_ENABLE_LTO "Enable Link Time Optimization" ON)
option(NCEI_NATIVE_ARCH "Use -march=native for CPU-specific tuning" OFF)
option(NCEI_ENABLE_NETCDF "Enable NetCDF format support" OFF)

# Aggressive optimization flags for Release builds
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

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

# CPU-specific tuning.
# NCEI_NATIVE_ARCH=ON → -march=native (only when you know the deploy CPU).
# Default: -march=x86-64-v3 (2017+ x86 baseline — AVX2, BMI2, FMA). Matches the
# service Dockerfiles so FetchContent-consumed builds produce identical codegen.
if(NCEI_NATIVE_ARCH)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
    message(STATUS "Using -march=native for CPU-specific optimizations")
else()
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-march=x86-64-v3" COMPILER_SUPPORTS_X86_64_V3)
    if(COMPILER_SUPPORTS_X86_64_V3)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=x86-64-v3")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mtune=generic")
    endif()
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
find_package(CURL REQUIRED)

include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(json)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add subdirectories
add_subdirectory(src)

if(NCEI_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
    )
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
