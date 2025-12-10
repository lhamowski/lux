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
        promise.then([&](int value) { value_from_promise_handler = value; });
        CHECK_FALSE(promise.resolved());

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

        promise.then([&](int) { CHECK(false); });
        CHECK_FALSE(promise.resolved());
        CHECK_THROWS_WITH(io.run(), "Test exception");
    }

    SECTION("Make resolved promise")
    {
        auto promise = lux::promise<int>{10};
        CHECK(promise.resolved());
        CHECK(promise.get() == 10);

        int value_from_handler = 0;
        promise.then([&value_from_handler](int val) { value_from_handler = val; });
        CHECK(value_from_handler == 10);
    }
}