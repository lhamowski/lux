#pragma once

#include <lux/io/coro/common.hpp>
#include <lux/support/move.hpp>
#include <lux/support/assert.hpp>

#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>

#include <chrono>
#include <optional>

namespace lux::coro {

/**
 * @brief An awaitable wrapper that adds timeout functionality to any awaitable task.
 *
 * The awaitable_deadline class wraps an existing awaitable and ensures it completes
 * within a specified time limit. If the wrapped task completes in time, the result
 * is returned as a success. If the timeout expires first, a timeout_tag failure is returned.
 *
 * @tparam T The return type of the wrapped awaitable.
 */
template <typename T>
class awaitable_deadline
{
public:
    /**
     * @brief Constructs a awaitable_deadline with the given task and timeout duration.
     *
     * @tparam Rep Duration representation type.
     * @tparam Period Duration period type.
     * @param task The awaitable task to wrap with timeout functionality.
     * @param timeout The timeout duration (converted to nanoseconds).
     */
    template <typename Rep, typename Period>
    awaitable_deadline(lux::coro::awaitable<T>&& task, std::chrono::duration<Rep, Period> timeout)
        : task_{lux::move(task)}, timeout_{std::chrono::duration_cast<std::chrono::nanoseconds>(timeout)}
    {
    }

    /**
     * @brief Returns an awaitable that can be used with co_spawn and other async operations.
     *
     * This method explicitly converts the awaitable_deadline to a standard awaitable,
     * making it compatible with boost::asio::co_spawn and similar functions.
     *
     * @return An awaitable that produces a std::optional<T>.
     */
    lux::coro::awaitable<std::optional<T>> as_awaitable() &&
    {
        return lux::move(*this).with_timeout();
    }

    /**
     * @brief Makes this class awaitable by providing the co_await operator.
     *
     * @return An awaitable that will complete with either the task result or a timeout.
     */
    auto operator co_await() &&
    {
        return lux::move(*this).with_timeout();
    }

private:
    /**
     * @brief Internal coroutine that implements the timeout logic.
     *
     * Runs the wrapped task and a timer concurrently. Whichever completes first
     * determines the final result.
     *
     * @return An awaitable that produces a std::optional<T>.
     */
    lux::coro::awaitable<std::optional<T>> with_timeout() &&
    {
        using namespace boost::asio::experimental::awaitable_operators;

        auto ex = co_await boost::asio::this_coro::executor;
        boost::asio::steady_timer timer{ex};
        timer.expires_after(timeout_);

        std::optional<T> result{};

        auto task = [&]() -> lux::coro::awaitable<void> {
            result = co_await lux::move(task_);
            timer.cancel();
        };

        auto timer_task = [&]() -> lux::coro::awaitable<void> {
            co_await timer.async_wait(boost::asio::use_awaitable);
        };

        co_await (task() || timer_task());
        co_return result;
    }

private:
    lux::coro::awaitable<T> task_;
    std::chrono::nanoseconds timeout_;
};

} // namespace lux::coro