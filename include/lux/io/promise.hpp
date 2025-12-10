#pragma once

#include <lux/support/move.hpp>
#include <lux/support/overload.hpp>

#include <boost/asio/experimental/promise.hpp>
#include <boost/asio/any_io_executor.hpp>

#include <exception>
#include <memory>
#include <utility>
#include <variant>

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
    using base_promise_type = lux::base_promise<T, Executor, Allocator>;

    promise(base_promise_type&& base) : promise_holder_{lux::move(base)}
    {
    }

    template <typename U>
    promise(U&& ready_value) : promise_holder_{std::forward<U>(ready_value)}
    {
    }

    // Wait for the promise to become ready and invoke the handler with the result.
    template <typename Handler>
    void then(Handler&& handler)
    {
        if (auto* base = std::get_if<base_promise_type>(&promise_holder_))
        {
            then_impl(*base, std::forward<Handler>(handler));
        }
        else if (auto* value = std::get_if<T>(&promise_holder_))
        {
            then_impl(*value, std::forward<Handler>(handler));
        }
    }

    T get()
    {
        T value;
        then([&value](T v) mutable { value = lux::move(v); });
        return value;
    }

    bool resolved() const noexcept
    {
        return std::holds_alternative<T>(promise_holder_);
    }

private:
    template <typename Handler>
    auto then_impl(base_promise_type& base, Handler&& handler)
    {
        base([handler = std::forward<Handler>(handler)](std::exception_ptr eptr, T value) mutable {
            if (eptr)
            {
                std::rethrow_exception(eptr);
            }
            handler(lux::move(value));
        });
    }

    template <typename Handler>
    void then_impl(T& value, Handler&& handler)
    {
        handler(lux::move(value));
    }

private:
    std::variant<base_promise_type, T> promise_holder_;
};



} // namespace lux