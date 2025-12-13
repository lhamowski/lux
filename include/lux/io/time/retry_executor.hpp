#pragma once

#include <lux/io/time/base/timer.hpp>
#include <lux/io/time/base/retry_policy.hpp>

#include <chrono>
#include <functional>

namespace lux::time {

class retry_executor
{
public:
    using retry_action_callback = std::function<void()>;
    using retry_exhausted_callback = std::function<void()>;

public:
    retry_executor(lux::time::base::timer_factory& timer_factory, const lux::time::base::retry_policy& policy);

    retry_executor(const retry_executor&) = delete;
    retry_executor& operator=(const retry_executor&) = delete;
    retry_executor(retry_executor&&) = default;
    retry_executor& operator=(retry_executor&&) = delete;

public:
    void set_retry_action(retry_action_callback action);
    void set_exhausted_callback(retry_exhausted_callback exhausted_callback);

    /**
     * @brief Execute the retry logic.
     * Execute the retry action and handle retries based on the configured strategy.
     */
    void retry();

    /**
     * @brief Reset the retry executor.
     * This will reset the internal state, allowing for a fresh retry operation.
     */
    void reset();

    /**
     * @brief Cancel any ongoing retry attempts.
     * This will stop the retry timer and prevent further retries
     */
    void cancel();

    /**
     * @brief Check if the retry attempts have been exhausted.
     * @return true if the maximum number of attempts has been reached or no more retries are allowed, false otherwise.
     */
    bool is_retry_exhausted() const;

private:
    void on_timer_expired();
    std::chrono::milliseconds calculate_next_delay() const;
    std::chrono::milliseconds calculate_linear_backoff_delay() const;
    std::chrono::milliseconds calculate_exponential_backoff_delay() const;

private:
    const lux::time::base::retry_policy policy_;
    retry_action_callback retry_action_;
    retry_exhausted_callback exhausted_callback_;

private:
    std::size_t attempts_{0};
    bool canceled_{false};
    lux::time::base::interval_timer_ptr timer_;
};

} // namespace lux::time