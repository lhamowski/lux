#include <lux/io/coro/awaitable_deadline.hpp>
#include <lux/io/coro/common.hpp>
#include <lux/support/move.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_future.hpp>

#include <chrono>

using namespace std::chrono_literals;

namespace {

lux::coro::awaitable<int> immediate_task()
{
    co_return 42;
}

lux::coro::awaitable<int> slow_task()
{
    auto executor = co_await boost::asio::this_coro::executor;
    boost::asio::steady_timer timer{executor};
    timer.expires_after(100ms);
    co_await timer.async_wait(lux::coro::use_awaitable);
    co_return 123;
}

} // namespace

TEST_CASE("Timeout awaitable basic tests", "[io][coro]")
{
    boost::asio::io_context io_context;

    SECTION("Task completes before timeout")
    {
        auto task = lux::coro::awaitable_deadline{immediate_task(), 50ms};
        auto future =
            lux::coro::co_spawn(io_context.get_executor(), lux::move(task).as_awaitable(), boost::asio::use_future);

        io_context.run();
        auto result = future.get();

        CHECK(result.has_value());
        CHECK(result.value() == 42);
    }

    SECTION("Task times out")
    {
        auto task = lux::coro::awaitable_deadline{slow_task(), 10ms};
        auto future =
            lux::coro::co_spawn(io_context.get_executor(), lux::move(task).as_awaitable(), boost::asio::use_future);

        io_context.run();
        auto result = future.get();

        CHECK(!result);
    }
}