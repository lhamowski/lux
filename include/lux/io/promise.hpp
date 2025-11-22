#pragma once

#include <boost/asio/experimental/promise.hpp>
#include <boost/asio/any_io_executor.hpp>

#include <memory>

namespace lux {

template <typename Signature = void(),
          typename Executor = boost::asio::any_io_executor,
          typename Allocator = std::allocator<void>>
using promise = boost::asio::experimental::promise<Signature, Executor, Allocator>;

}