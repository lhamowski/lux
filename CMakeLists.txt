cmake_minimum_required(VERSION 3.30)
project(lux LANGUAGES CXX VERSION 0.1)

list(APPEND CMAKE_MODULE_PATH ${lux_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH})

include(CMakeDependentOption)
include(CFlex)
include(LuxSourceGroup)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(lux_master_project ON)
else()
    set(lux_master_project OFF)
endif()

if(lux_master_project)
    set(CMAKE_CXX_STANDARD 20)
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
option(LUX_FETCH_DEPS "Fetch required dependencies" ON)

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
