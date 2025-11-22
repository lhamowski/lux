#pragma once

#include <lux/io/promise.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/experimental/promise.hpp>
#include <boost/asio/experimental/use_promise.hpp>

#include <concepts>
#include <utility>

namespace lux::coro {

template <typename T>
using awaitable = boost::asio::awaitable<T>;

using boost::asio::co_spawn;

inline constexpr auto detached = boost::asio::detached;
inline constexpr auto use_awaitable = boost::asio::use_awaitable;
inline constexpr auto use_future = boost::asio::use_future;
inline constexpr auto use_base_promise = boost::asio::experimental::use_promise;

namespace detail {

struct use_promise_t
{
};

} // namespace detail

inline constexpr auto use_promise = detail::use_promise_t{};

template <typename Executor, typename F, std::same_as<detail::use_promise_t> CompletionToken>
inline auto co_spawn(const Executor& exe, F&& f, [[maybe_unused]] CompletionToken token)
{
    return lux::promise{lux::coro::co_spawn(exe, std::forward<F>(f), lux::coro::use_base_promise)};
}

} // namespace lux::coro