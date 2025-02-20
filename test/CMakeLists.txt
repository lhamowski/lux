include(CFlex)

cflex_add_executable(lux-test SOURCES
    lux_test.cpp
)

target_link_libraries(lux-test PRIVATE lux::lux Catch2::Catch2WithMain)
set_target_properties(lux-test PROPERTIES FOLDER lux)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
include(Catch)
include(SourceGroup)

kl_source_group(lux-test TREE ${lux_SOURCE_DIR})
catch_discover_tests(lux-test)

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    include(CodeCoverage)
    kl_add_coverage_target_gcovr(lux-coverage
        COMMAND lux-test
        FILTERS
           ${lux_SOURCE_DIR}/src/
           ${lux_SOURCE_DIR}/include/
    )
endif()
