#include <lux/io/time/retry_executor.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/move.hpp>

#include <algorithm>

namespace lux::time {

retry_executor::retry_executor(lux::time::base::timer_factory& timer_factory,
                               const lux::time::base::retry_policy& policy)
    : policy_{policy}, timer_{timer_factory.create_interval_timer()}
{
    timer_->set_handler([this]() { on_timer_expired(); });
}

void retry_executor::set_retry_action(retry_action_callback action)
{
    retry_action_ = lux::move(action);
}

void retry_executor::set_exhausted_callback(retry_exhausted_callback exhausted_callback)
{
    exhausted_callback_ = lux::move(exhausted_callback);
}

void retry_executor::retry()
{
    if (max_attempts_reached())
    {
        // If we've reached the maximum attempts, we do not schedule further retries.
        return;
    }

    const auto next_delay = calculate_next_delay();
    if (next_delay == std::chrono::milliseconds::zero())
    {
        // If the next delay is zero, we call the action immediately.
        on_timer_expired();
    }
    else
    {
        timer_->schedule(next_delay);
    }
}

void retry_executor::reset()
{
    timer_->cancel();
    attempts_ = 0;
}

void retry_executor::on_timer_expired()
{
    ++attempts_;

    if (retry_action_)
    {
        retry_action_();
    }

    if (max_attempts_reached())
    {
        if (exhausted_callback_)
        {
            exhausted_callback_();
        }
    }
}

std::chrono::milliseconds retry_executor::calculate_next_delay() const
{
    switch (policy_.strategy)
    {
    case lux::time::base::retry_policy::backoff_strategy::fixed_delay:
        return std::min(policy_.base_delay, policy_.max_delay);
    case lux::time::base::retry_policy::backoff_strategy::linear_backoff:
        return calculate_linear_backoff_delay();
    case lux::time::base::retry_policy::backoff_strategy::exponential_backoff:
        return calculate_exponential_backoff_delay();
    }

    LUX_UNREACHABLE();
}

std::chrono::milliseconds retry_executor::calculate_linear_backoff_delay() const
{
    LUX_ASSERT(policy_.strategy == lux::time::base::retry_policy::backoff_strategy::linear_backoff,
               "This method should only be called for linear backoff strategy");

    if (attempts_ == 0)
    {
        return std::min(policy_.base_delay, policy_.max_delay);
    }

    if (policy_.base_delay.count() == 0)
    {
        return std::chrono::milliseconds::zero();
    }

    constexpr auto max_ms = static_cast<std::size_t>(std::chrono::milliseconds::max().count());
    const auto base_ms = static_cast<std::size_t>(policy_.base_delay.count());

    // We ensure that the calculated delay does not exceed the maximum allowed delay.
    // It checks base_ms * attempts_ > max_ms but without potential overflow.
    if (base_ms > max_ms / attempts_)
    {
        return policy_.max_delay;
    }

    const auto calculated_delay = std::chrono::milliseconds{base_ms * attempts_};
    const auto final_delay = std::min(calculated_delay, policy_.max_delay);
    return final_delay;
}

std::chrono::milliseconds retry_executor::calculate_exponential_backoff_delay() const
{
    LUX_ASSERT(policy_.strategy == lux::time::base::retry_policy::backoff_strategy::exponential_backoff,
               "This method should only be called for exponential backoff strategy");

    if (attempts_ == 0)
    {
        return std::min(policy_.base_delay, policy_.max_delay);
    }

    if (policy_.base_delay.count() == 0)
    {
        return std::chrono::milliseconds::zero();
    }

    // Maximum bit shift for size_t (63 for 64-bit systems)
    constexpr auto max_bit_shift = sizeof(attempts_) * 8 - 1;

    const auto bit_shift = attempts_;
    if (bit_shift > max_bit_shift)
    {
        // Avoid overflow by capping the delay at max_delay
        return policy_.max_delay;
    }

    const auto multiplier = std::size_t{1} << bit_shift; // 2 ^ (attempts_ - 1)
    constexpr auto max_ms = static_cast<std::size_t>(std::chrono::milliseconds::max().count());
    const auto base_ms = static_cast<std::size_t>(policy_.base_delay.count());

    // We ensure that the calculated delay does not exceed the maximum allowed delay.
    // It checks base_ms * multiplier > max_ms but without potential overflow.
    if (base_ms > max_ms / multiplier)
    {
        return policy_.max_delay;
    }

    const auto calculated_delay = std::chrono::milliseconds{base_ms * multiplier};
    const auto final_delay = std::min(calculated_delay, policy_.max_delay);
    return final_delay;
}

bool retry_executor::max_attempts_reached() const
{
    // If max_attempts is set to std::nullopt, it means infinite attempts are allowed.
    return policy_.max_attempts && attempts_ >= *policy_.max_attempts;
}

} // namespace lux::time