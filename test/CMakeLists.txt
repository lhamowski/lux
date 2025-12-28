cflex_add_executable(lux-test SOURCES
    logger/logger_test.cpp
    
    support/container_test.cpp
    support/enum_test.cpp
    support/exception_test.cpp
    support/finally_test.cpp
    support/move_test.cpp
    support/hash_test.cpp
    support/path_test.cpp
    support/result_test.cpp
    support/scoped_value_test.cpp
    support/strong_typedef_test.cpp

    utils/buffer_writer_test.cpp
    utils/buffer_reader_test.cpp
    utils/expiring_ref_test.cpp
    utils/lifetime_guard_test.cpp
    utils/memory_arena_test.cpp
    utils/stopwatch_test.cpp
)

target_link_libraries(lux-test 
    PRIVATE 
        Catch2::Catch2WithMain
        lux::lux 
        fmt::fmt
)

if(LUX_ENABLE_IO)
    target_sources(lux-test PRIVATE
        io/net/test_utils.hpp

        io/net/address_v4_test.cpp
        io/net/endpoint_test.cpp
        io/net/http_factory_test.cpp
        io/net/http_server_test.cpp
        io/net/socket_factory_test.cpp
        io/net/tcp_acceptor_test.cpp
        io/net/tcp_inbound_socket_test.cpp
        io/net/tcp_socket_test.cpp
        io/net/udp_socket_test.cpp

        io/time/retry_executor_test.cpp
        io/time/interval_timer_test.cpp
        io/time/timer_factory_test.cpp

        io/time/mocks/interval_timer_mock.hpp
        io/time/mocks/timer_factory_mock.hpp

        io/coro/algorithms_test.cpp
        io/coro/awaitable_event_test.cpp

        io/promise_test.cpp
    )
    target_link_libraries(lux-test 
        PRIVATE 
            lux::io)
endif()

set_target_properties(lux-test PROPERTIES FOLDER lux)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
include(Catch)

lux_source_group(lux-test TREE ${lux_SOURCE_DIR})
catch_discover_tests(lux-test)
