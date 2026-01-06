cmake_minimum_required(VERSION 3.30)
project(lux LANGUAGES CXX VERSION 0.1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CMakeDependentOption)
include(CFlex)
include(LuxSourceGroup)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(lux_master_project ON)
else()
    set(lux_master_project OFF)
endif()

if(lux_master_project)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

cmake_dependent_option(
    LUX_TEST
    "Generate the test target."
    ON
    "${lux_master_project}"
    OFF
)
option(LUX_ENABLE_IO "Enable lux-io module" ON)
option(LUX_ENABLE_CRYPTO "Enable lux-crypto module" ON)
option(LUX_FETCH_DEPS "Fetch required dependencies" ON)
option(LUX_FMT_EXTERNAL "Use external fmt library" OFF)

if(LUX_ENABLE_IO)
    set(Boost_USE_STATIC_LIBS ON)
    find_package(Boost 1.87.0 REQUIRED COMPONENTS url)
    if (${Boost_FOUND})
        message(STATUS "Found Boost ${Boost_VERSION}")
    endif()
endif()

if(LUX_FETCH_DEPS)
    add_subdirectory(external)
else()
    if(LUX_TEST)
        find_package(Catch2 3.8 REQUIRED)
    endif()
endif()

add_subdirectory(src)

if(LUX_TEST)
    enable_testing()
    add_subdirectory(test)
endif()
