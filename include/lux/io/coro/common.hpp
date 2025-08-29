#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace lux::coro {

template <typename T>
using awaitable = boost::asio::awaitable<T>;

using boost::asio::co_spawn;

inline constexpr auto detached = boost::asio::detached;
inline constexpr auto use_awaitable = boost::asio::use_awaitable;

} // namespace lux::coro