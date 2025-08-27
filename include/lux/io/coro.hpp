#pragma once

#include <lux/support/concepts.hpp>
#include <lux/support/move.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/experimental/channel.hpp>

namespace lux::coro {

template <typename T>
using awaitable = boost::asio::awaitable<T>;

using boost::asio::co_spawn;

inline constexpr auto detached = boost::asio::detached;
inline constexpr auto use_awaitable = boost::asio::use_awaitable;

/**
 * @brief Creates a vector of awaitables by applying a generator functor to each element.
 *
 * @tparam Container  Range of elements (rvalue).
 * @tparam Generator  Functor taking element and returning lux::coro::awaitable<T>.
 * @param elements    Rvalue container of elements.
 * @param gen         Functor generating awaitable from element.
 * @return std::vector<lux::coro::awaitable<T>>  Vector of awaitables ready to co_await.
 */
template <typename Container, typename Generator>
auto make_tasks(Container&& elements, Generator&& gen)
{
    using task_t = decltype(gen(*std::begin(elements)));
    std::vector<task_t> tasks;
    tasks.reserve(elements.size());

    for (auto&& elem : elements)
    {
        tasks.push_back(gen(std::forward<decltype(elem)>(elem)));
    }

    return tasks;
}

/**
 * @brief Execute multiple awaitables concurrently and return when any predicate succeeds.
 *
 * Spawns all awaitables from the given range, evaluates each result with the provided
 * predicate, and completes once one of them returns true. If none succeed, returns false.
 *
 * @tparam Range Rvalue range of awaitables (must be passed as rvalue).
 * @tparam Pred  Predicate taking the result of each awaitable and returning bool.
 * @param range  Rvalue range of awaitables to execute.
 * @param pred   Predicate applied to each task result.
 * @return lux::coro::awaitable<bool>  True if any predicate succeeded, false otherwise.
 */
template <lux::rvalue Range, lux::rvalue Pred>
lux::coro::awaitable<bool> when_any(Range&& range, Pred&& pred)
{
    if (range.empty())
    {
        co_return false;
    }

    auto exe = co_await boost::asio::this_coro::executor;
    boost::asio::experimental::channel<void(boost::system::error_code, bool)> results{exe, range.size()};

    for (auto&& task : range)
    {
        co_spawn(
            exe,
            [task = lux::move(task), &results, &pred]() mutable -> awaitable<void> {
                const auto task_result = co_await lux::move(task);
                const bool result = pred(task_result);
                co_await results.async_send(boost::system::error_code{}, result, use_awaitable);
            },
            detached);
    }

    for (std::size_t i = 0; i < range.size(); ++i)
    {
        if (co_await results.async_receive(use_awaitable))
        {
            co_return true;
        }
    }

    co_return false;
}

} // namespace lux::coro
