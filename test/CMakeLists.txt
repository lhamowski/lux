include(CFlex)

cflex_add_executable(lux-test SOURCES
    logger_test.cpp
)

target_link_libraries(lux-test PRIVATE 
    Catch2::Catch2WithMain
    lux::lux 
    fmt::fmt
    spdlog::spdlog
)
set_target_properties(lux-test PROPERTIES FOLDER lux)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
include(Catch)
include(SourceGroup)

kl_source_group(lux-test TREE ${lux_SOURCE_DIR})
catch_discover_tests(lux-test)

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    include(CodeCoverage)
    kl_add_coverage_target_lcov(lux-coverage
        COMMAND lux-test
        OUTPUT_NAME coverage
        FILTERS
           ${lux_SOURCE_DIR}/src/
           ${lux_SOURCE_DIR}/include/
        LCOV_ARGS 
            --gcov-tool llvm-cov-gcov
             # see https://github.com/linux-test-project/lcov/issues/220
            --erase-functions __cxx_global_var_init
    )
endif()
