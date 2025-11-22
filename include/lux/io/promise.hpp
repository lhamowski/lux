#pragma once

#include <lux/support/move.hpp>

#include <boost/asio/experimental/promise.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/async_result.hpp>

#include <exception>
#include <memory>

namespace lux {

template <typename T, typename Executor = boost::asio::any_io_executor, typename Allocator = std::allocator<void>>
using base_promise = boost::asio::experimental::promise<void(std::exception_ptr, T), Executor, Allocator>;

/**
 * @brief A simplified promise wrapper for asynchronous operations that handle exceptions by rethrowing.
 *
 * @tparam T The type of the value to be delivered when the promise is ready.
 * @tparam Executor The executor type used for asynchronous operations. Defaults to boost::asio::any_io_executor.
 * @tparam Allocator The allocator type used for memory management. Defaults to std::allocator<void>.
 *
 * @note This class wraps a base promise that uses a completion signature of void(std::exception_ptr, T). If an
 * exception occurs, it is rethrown to the caller (e.g. to the exection context event loop).
 */
template <typename T, typename Executor = boost::asio::any_io_executor, typename Allocator = std::allocator<void>>
class promise
{
public:
    promise(lux::base_promise<T, Executor, Allocator>&& base) : promise_{lux::move(base)}
    {
    }

    // Wait for the promise to become ready.
    template <BOOST_ASIO_COMPLETION_TOKEN_FOR(void(T)) CompletionToken>
    auto async_wait(CompletionToken&& token)
    {
        auto initiation = [this](auto&& handler) {
            return promise_([h = std::forward<decltype(handler)>(handler)](std::exception_ptr ep, T value) {
                if (ep)
                {
                    std::rethrow_exception(ep);
                }

                h(value);
            });
        };

        return boost::asio::async_initiate<CompletionToken, void(T)>(initiation, token);
    }

    void cancel()
    {
        promise_.cancel();
    }

    bool completed() const
    {
        return promise_.completed();
    }

private:
    base_promise<T, Executor, Allocator> promise_;
};

} // namespace lux