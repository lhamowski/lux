#include "test_case.hpp"

#include <lux/io/promise.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/experimental/use_promise.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <future>
#include <string>

using namespace std::chrono_literals;

namespace {

constexpr auto use_promise = boost::asio::experimental::use_promise;

} // namespace

LUX_TEST_CASE("promise", "constructs from base_promise and resolves with value", "[io][promise]")
{
    boost::asio::io_context io;

    SECTION("Resolves with integer value")
    {
        auto promise = boost::asio::co_spawn(
            io.get_executor(),
            []() -> boost::asio::awaitable<int> { co_return 42; },
            use_promise);

        auto lux_promise = lux::promise<int>{std::move(promise)};
        CHECK_FALSE(lux_promise.resolved());

        int captured_value{0};
        lux_promise.then([&](int value) { captured_value = value; });

        io.run();

        CHECK(captured_value == 42);
    }

    SECTION("Resolves with string value")
    {
        auto promise = boost::asio::co_spawn(
            io.get_executor(),
            []() -> boost::asio::awaitable<std::string> { co_return std::string{"hello"}; },
            use_promise);

        auto lux_promise = lux::promise<std::string>{std::move(promise)};
        CHECK_FALSE(lux_promise.resolved());

        std::string captured_value;
        lux_promise.then([&](std::string value) { captured_value = std::move(value); });

        io.run();

        CHECK(captured_value == "hello");
    }

    SECTION("Resolves asynchronously after delay")
    {
        boost::asio::steady_timer timer{io, 10ms};

        auto promise = boost::asio::co_spawn(
            io.get_executor(),
            [&]() -> boost::asio::awaitable<int> {
                co_await timer.async_wait(boost::asio::use_awaitable);
                co_return 123;
            },
            use_promise);

        auto lux_promise = lux::promise<int>{std::move(promise)};

        int captured_value{0};
        lux_promise.then([&](int value) { captured_value = value; });

        io.run();

        CHECK(captured_value == 123);
    }
}

LUX_TEST_CASE("promise", "constructs with ready value and resolves immediately", "[io][promise]")
{
    SECTION("Constructs with integer")
    {
        auto promise = lux::promise<int>{42};

        CHECK(promise.resolved());
        CHECK(promise.get() == 42);
    }

    SECTION("Constructs with string")
    {
        auto promise = lux::promise<std::string>{std::string{"ready"}};

        CHECK(promise.resolved());
        CHECK(promise.get() == "ready");
    }

    SECTION("Then handler called immediately for ready value")
    {
        auto promise = lux::promise<int>{99};

        int captured_value{0};
        promise.then([&](int value) { captured_value = value; });

        CHECK(captured_value == 99);
    }
}

LUX_TEST_CASE("promise", "rethrows exception from async operation", "[io][promise]")
{
    boost::asio::io_context io;

    SECTION("Rethrows runtime_error")
    {
        auto promise = boost::asio::co_spawn(
            io.get_executor(),
            []() -> boost::asio::awaitable<int> {
                throw std::runtime_error("async error");
                co_return 0;
            },
            use_promise);

        auto lux_promise = lux::promise<int>{std::move(promise)};
        lux_promise.then([](int) { FAIL("Handler should not be called"); });

        CHECK_THROWS_WITH(io.run(), "async error");
    }

    SECTION("Rethrows logic_error")
    {
        auto promise = boost::asio::co_spawn(
            io.get_executor(),
            []() -> boost::asio::awaitable<std::string> {
                throw std::logic_error("invalid operation");
                co_return "";
            },
            use_promise);

        auto lux_promise = lux::promise<std::string>{std::move(promise)};
        lux_promise.then([](std::string) { FAIL("Handler should not be called"); });

        CHECK_THROWS_WITH(io.run(), "invalid operation");
    }
}

LUX_TEST_CASE("promise", "extracts value using get()", "[io][promise]")
{
    SECTION("Gets value from resolved promise immediately")
    {
        auto promise = lux::promise<int>{888};
        CHECK(promise.get() == 888);
    }
}
