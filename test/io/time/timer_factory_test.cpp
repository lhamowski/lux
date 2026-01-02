#include "test_case.hpp"

#include <lux/io/time/timer_factory.hpp>

#include <catch2/catch_all.hpp>

LUX_TEST_CASE("timer_factory", "creates interval timer successfully", "[io][time]")
{
    boost::asio::io_context io_context;

    lux::time::timer_factory factory{io_context.get_executor()};
    auto timer = factory.create_interval_timer();

    REQUIRE(timer != nullptr);
}

