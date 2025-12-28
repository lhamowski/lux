#include <lux/io/coro/awaitable_event.hpp>
#include <lux/io/coro/common.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_future.hpp>

#include <chrono>

using namespace std::chrono_literals;

TEST_CASE("awaitable_event: completes with delivered value", "[io][coro]")
{
    boost::asio::io_context io;

    SECTION("Completes with delivered value")
    {
        lux::coro::awaitable_event<int> ev;
        int value_from_promise_handler = 0;

        auto fut = boost::asio::co_spawn(
            io.get_executor(),
            [&]() -> lux::coro::awaitable<void> {
                auto promise = ev.async_wait();
                co_return promise([&](int value) { value_from_promise_handler = value; });
            },
            boost::asio::use_future);

        boost::asio::steady_timer t{io};
        t.expires_after(10ms);
        t.async_wait([&](auto) { ev.trigger(123); });

        io.run();
        io.restart();

        CHECK(value_from_promise_handler == 123);
    }

    SECTION("Trigger without waiter is a no-op, later wait gets later trigger")
    {
        lux::coro::awaitable_event<int> ev;

        // Trigger before anyone is waiting
        ev.trigger(1);

        auto fut = boost::asio::co_spawn(
            io.get_executor(),
            [&]() -> lux::coro::awaitable<int> { co_return co_await ev.async_wait(); },
            boost::asio::use_future);

        boost::asio::steady_timer t{io};
        t.expires_after(10ms);
        t.async_wait([&](auto) { ev.trigger(456); });

        io.run();
        io.restart();

        CHECK(fut.get() == 456);
    }

    SECTION("Can be reused after completion (single-shot semantics)")
    {
        lux::coro::awaitable_event<int> ev;

        auto fut1 = boost::asio::co_spawn(
            io.get_executor(),
            [&]() -> lux::coro::awaitable<int> { co_return co_await ev.async_wait(); },
            boost::asio::use_future);

        boost::asio::steady_timer t1{io};
        t1.expires_after(5ms);
        t1.async_wait([&](auto) { ev.trigger(10); });

        io.run();
        io.restart();

        CHECK(fut1.get() == 10);

        // Second wait/trigger after handler was cleared
        auto fut2 = boost::asio::co_spawn(
            io.get_executor(),
            [&]() -> lux::coro::awaitable<int> { co_return co_await ev.async_wait(); },
            boost::asio::use_future);

        boost::asio::steady_timer t2{io};
        t2.expires_after(5ms);
        t2.async_wait([&](auto) { ev.trigger(20); });

        io.run();
        io.restart();

        CHECK(fut2.get() == 20);
    }
}

TEST_CASE("awaitable_event: completes when triggered without value", "[io][coro]")
{
    boost::asio::io_context io;

    SECTION("Completes when triggered")
    {
        lux::coro::awaitable_event<void> ev;

        auto fut = boost::asio::co_spawn(
            io.get_executor(),
            [&]() -> lux::coro::awaitable<void> { co_await ev.async_wait(); },
            boost::asio::use_future);

        boost::asio::steady_timer t{io};
        t.expires_after(10ms);
        t.async_wait([&](auto) { ev.trigger(); });

        io.run();
        io.restart();

        CHECK_NOTHROW(fut.get());
    }

    SECTION("Can be reused after completion")
    {
        lux::coro::awaitable_event<void> ev;

        auto fut1 = boost::asio::co_spawn(
            io.get_executor(),
            [&]() -> lux::coro::awaitable<void> { co_await ev.async_wait(); },
            boost::asio::use_future);

        boost::asio::steady_timer t1{io};
        t1.expires_after(5ms);
        t1.async_wait([&](auto) { ev.trigger(); });

        io.run();
        io.restart();

        CHECK_NOTHROW(fut1.get());

        auto fut2 = boost::asio::co_spawn(
            io.get_executor(),
            [&]() -> lux::coro::awaitable<void> { co_await ev.async_wait(); },
            boost::asio::use_future);

        boost::asio::steady_timer t2{io};
        t2.expires_after(5ms);
        t2.async_wait([&](auto) { ev.trigger(); });

        io.run();
        io.restart();

        CHECK_NOTHROW(fut2.get());
    }
}