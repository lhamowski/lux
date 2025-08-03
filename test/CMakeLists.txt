cflex_add_executable(lux-test SOURCES
    logger/logger_test.cpp
    
    support/enum_test.cpp
    support/exception_test.cpp
    support/finally_test.cpp
    support/path_test.cpp
    support/strong_typedef_test.cpp

    utils/memory_arena_test.cpp
    utils/stopwatch_test.cpp
)

target_link_libraries(lux-test 
    PRIVATE 
        Catch2::Catch2WithMain
        lux::lux 
        fmt::fmt
)

if(LUX_ENABLE_NET)
    target_sources(lux-test PRIVATE
        net/address_v4_test.cpp
        net/endpoint_test.cpp
        net/udp_socket_test.cpp
    )
    target_link_libraries(lux-test 
        PRIVATE 
            lux::net)
endif()

set_target_properties(lux-test PROPERTIES FOLDER lux)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
include(Catch)

lux_source_group(lux-test TREE ${lux_SOURCE_DIR})
catch_discover_tests(lux-test)
