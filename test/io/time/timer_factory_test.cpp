#include <lux/io/time/timer_factory.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("Timer factory creates interval timer", "[io][time]")
{
    boost::asio::io_context io_context;

    lux::time::timer_factory factory{io_context.get_executor()};
    auto timer = factory.create_interval_timer();

    REQUIRE(timer != nullptr);
}
