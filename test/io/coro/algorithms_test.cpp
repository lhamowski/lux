#include <lux/io/coro/algorithms.hpp>
#include <lux/support/move.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include <vector>

TEST_CASE("coro_algorithms: when_any checks if any task satisfies predicate", "[io][coro]")
{
    boost::asio::io_context io_context;

    SECTION("when_any returns true if any awaitable satisfies the predicate")
    {
        auto tasks = lux::coro::make_tasks(std::vector<int>{1, 2, 3},
                                           [](int value) -> lux::coro::awaitable<int> { co_return value; });

        auto result = lux::coro::co_spawn(io_context.get_executor(),
                                          lux::coro::when_any(lux::move(tasks), [](int value) { return value == 2; }),
                                          boost::asio::use_future);

        io_context.run();
        CHECK(result.get() == true);
    }

    SECTION("when_any returns false if no awaitable satisfies the predicate")
    {
        auto tasks = lux::coro::make_tasks(std::vector<int>{1, 2, 3},
                                           [](int value) -> lux::coro::awaitable<int> { co_return value; });

        auto result = lux::coro::co_spawn(io_context.get_executor(),
                                          lux::coro::when_any(lux::move(tasks), [](int value) { return value == 4; }),
                                          boost::asio::use_future);
        io_context.run();
        CHECK(result.get() == false);
    }
}