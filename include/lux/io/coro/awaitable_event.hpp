#pragma once

#include <lux/io/coro/common.hpp>
#include <lux/support/move.hpp>

#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/async_result.hpp>

namespace lux::coro {

/**
 * @brief Single-shot awaitable event delivering a value to a waiting coroutine.
 *
 * Allows one consumer to asynchronously wait for a value of type T using
 * lux::coro::use_awaitable. Once triggered, the stored handler is invoked and cleared.
 *
 * @tparam T Value type delivered to the awaiting handler.
 */
template <typename T>
class awaitable_event
{
public:
    /**
     * @brief Asynchronously wait for the event to be triggered.
     *
     * Stores the completion handler and returns an awaitable that completes when
     * trigger(T) is called. This is a single-shot wait; after invocation the handler is reset.
     *
     * @return lux::coro::awaitable<T> awaitable that yields the delivered value.
     *
     * Completion signature: void(T)
     */
    lux::coro::awaitable<T> async_wait()
    {
        return boost::asio::async_initiate<decltype(lux::coro::use_awaitable), void(T)>(
            [&](auto h) { handler_ = lux::move(h); }, lux::coro::use_awaitable);
    }

    /**
     * @brief Triggers the event, delivering the given value to the stored handler.
     *
     * If a handler is stored, it is invoked with the provided value and then cleared.
     * If no handler is stored, this call is a no-op.
     *
     * @param value The value to deliver to the awaiting handler.
     */
    void trigger(T value)
    {
        if (handler_)
        {
            handler_(lux::move(value));
        }

        handler_ = {};
    }

private:
    boost::asio::any_completion_handler<void(T)> handler_;
};

/**
 * @brief Specialization for events without a value.
 *
 * Represents a single-shot awaitable event that signals completion without delivering a value.
 */
template <>
class awaitable_event<void>
{
public:
    /**
     * @brief Asynchronously wait for the event to be triggered.
     *
     * Stores the completion handler and returns an awaitable that completes when trigger() is called.
     * Single-shot; the handler is cleared after invocation.
     *
     * @return lux::coro::awaitable<void> awaitable that completes on trigger().
     *
     * Completion signature: void()
     */
    lux::coro::awaitable<void> async_wait()
    {
        return boost::asio::async_initiate<decltype(lux::coro::use_awaitable), void()>(
            [&](auto h) { handler_ = lux::move(h); }, lux::coro::use_awaitable);
    }

    /**
     * @brief Triggers the event, invoking the stored handler without a value.
     *
     * If a handler is stored, it is invoked and then cleared. If no handler is stored, this call is a no-op.
     */
    void trigger()
    {
        if (handler_)
        {
            handler_();
        }

        handler_ = {};
    }

private:
    boost::asio::any_completion_handler<void()> handler_;
};

} // namespace lux::coro