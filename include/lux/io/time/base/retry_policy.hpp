#pragma once

#include <chrono>
#include <cstddef>
#include <optional>

namespace lux::time::base {

struct retry_policy
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
     * If set to std::nullopt, the retry mechanism will continue indefinitely until successful or stopped.
     */
    std::optional<std::size_t> max_attempts;

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

} // namespace lux::time::base