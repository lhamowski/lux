#include <lux/io/promise.hpp>
#include <lux/io/coro/common.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/io_context.hpp>

TEST_CASE("lux::promise<T> behavior", "[io][promise]")
{
    boost::asio::io_context io;

    SECTION("Completes with value")
    {
        auto promise = lux::coro::co_spawn(
            io.get_executor(),
            [&]() -> lux::coro::awaitable<int> { co_return 42; },
            lux::coro::use_promise);

        int value_from_promise_handler = 0;
        promise.async_wait([&](int value) { value_from_promise_handler = value; });

        io.run();
        io.restart();

        CHECK(value_from_promise_handler == 42);
    }

    SECTION("Completes with exception")
    {
        auto promise = lux::coro::co_spawn(
            io.get_executor(),
            [&]() -> lux::coro::awaitable<int> { throw std::runtime_error("Test exception"); },
            lux::coro::use_promise);

        promise.async_wait([&](int) { CHECK(false); });
        CHECK_THROWS_WITH(io.run(), "Test exception");
    }
}