cmake_minimum_required(VERSION 3.30)
project(lux LANGUAGES CXX VERSION 0.1)

list(APPEND CMAKE_MODULE_PATH ${lux_SOURCE_DIR};${CMAKE_MODULE_PATH})

include(CMakeDependentOption)
include(cmake/LuxSourceGroup)

# Force local inclusion of CFlex to avoid conflicts with external projects
# that might also provide a CFlex module. By using a relative path instead
# of relying on CMAKE_MODULE_PATH, we ensure that lux always uses its own
# version of CFlex, maintaining consistent behavior regardless of how
# the project is integrated as a dependency.
include(cmake/CFlex)

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
option(LUX_ENABLE_NET "Enable lux-net module" ON)
option(LUX_FETCH_DEPS "Fetch required dependencies" ON)
option(LUX_FMT_EXTERNAL "Use external fmt library" OFF)

if(LUX_ENABLE_NET)
    find_package(Boost 1.87.0 REQUIRED)
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
