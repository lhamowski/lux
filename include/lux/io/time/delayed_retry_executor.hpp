#pragma once

#include <lux/io/time/base/timer.hpp>

#include <chrono>
#include <functional>

namespace lux::time {

struct delayed_retry_config
{
    enum class backoff_strategy
    {
        fixed_delay,
        linear_backoff,
        exponential_backoff,
    };

    /**
     * @brief Strategy for retrying operations.
     * This defines how the retry mechanism will behave in terms of delay between attempts.
     * - fixed_delay: The delay between attempts is constant.
     * - exponential_backoff: The delay increases exponentially with each attempt.
     * - linear_backoff: The delay increases linearly with each attempt.
     */
    backoff_strategy strategy{backoff_strategy::exponential_backoff};

    /**
     * @brief Maximum number of retry attempts.
     * If set to 0, it will retry indefinitely.
     */
    std::size_t max_attempts{5};

    /**
     * @brief Base delay before the first retry attempt.
     * This is used as the base delay for the retry mechanism.
     */
    std::chrono::milliseconds base_delay{1000};

    /**
     * @brief Maximum delay between retry attempts.
     * This is used as a ceiling for the delay, regardless of the backoff strategy.
     */
    std::chrono::milliseconds max_delay{30000};
};

class delayed_retry_executor
{
public:
    using retry_action_callback = std::function<void()>;
    using retry_exhausted_callback = std::function<void()>;

public:
    delayed_retry_executor(lux::time::base::timer_factory& timer_factory, const delayed_retry_config& config);

    delayed_retry_executor(const delayed_retry_executor&) = delete;
    delayed_retry_executor& operator=(const delayed_retry_executor&) = delete;
    delayed_retry_executor(delayed_retry_executor&&) = default;
    delayed_retry_executor& operator=(delayed_retry_executor&&) = delete;

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

private:
    void on_timer_expired();
    std::chrono::milliseconds calculate_next_delay() const;
    std::chrono::milliseconds calculate_linear_backoff_delay() const;
    std::chrono::milliseconds calculate_exponential_backoff_delay() const;

private:
    const delayed_retry_config config_;
    retry_action_callback retry_action_;
    retry_exhausted_callback exhausted_callback_;

private:
    std::size_t attempts_{0};
    lux::time::base::interval_timer_ptr timer_;
};

} // namespace lux::time