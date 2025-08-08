#pragma once

#include <chrono>
#include <functional>
#include <memory>

namespace lux::time::base {

/**
 * @brief An interface for an interval timer that can be scheduled to run at specific intervals.
 *
 * This interface defines the basic operations for an interval timer, including setting a handler,
 * scheduling a one-time execution, scheduling periodic executions, and canceling the timer.
 */
class interval_timer
{
public:
    /**
     * @brief Sets the handler to be called when the timer expires.
     *
     * @param callback The function to be called when the timer expires.
     */
    virtual void set_handler(std::function<void()> callback) = 0;

    /**
     * @brief Schedules the timer to run once after a specified delay.
     *
     * @param timeout The delay after which the timer should expire.vs
     */
    virtual void schedule(std::chrono::milliseconds delay) = 0;

    /**
     * @brief Schedules the timer to run periodically at a specified interval.
     *
     * @param interval The interval at which the timer should expire.
     */
    virtual void schedule_periodic(std::chrono::milliseconds interval) = 0;

    /**
     * @brief Cancels the timer, preventing any further execution.
     *
     * This method stops the timer and ensures that no handler will be called after this point.
     */
    virtual void cancel() = 0;

public:
    virtual ~interval_timer() = default;
};

using interval_timer_ptr = std::unique_ptr<interval_timer>;

/**
 * @brief A factory interface for creating interval timers.
 *
 * This interface allows for the creation of interval timers, which can be used to schedule tasks
 * at specific intervals or after a delay.
 */
class timer_factory
{
public:
    /**
     * @brief Creates a new interval timer instance.
     *
     * This method should return a pointer to a newly created interval timer.
     *
     * @return A unique pointer to the created interval timer.
     */
    virtual lux::time::base::interval_timer_ptr create_interval_timer() = 0;

public:
    virtual ~timer_factory() = default;
};

} // namespace lux::time::base