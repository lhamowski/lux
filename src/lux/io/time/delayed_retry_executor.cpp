#include <lux/io/time/delayed_retry_executor.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/move.hpp>

#include <algorithm>

namespace lux::time {

delayed_retry_executor::delayed_retry_executor(lux::time::base::timer_factory& timer_factory,
                                               const delayed_retry_config& config)
    : config_{config}, timer_{timer_factory.create_interval_timer()}
{
    timer_->set_handler([this]() { on_timer_expired(); });
}

void delayed_retry_executor::set_retry_action(retry_action_callback action)
{
    retry_action_ = lux::move(action);
}

void delayed_retry_executor::set_exhausted_callback(retry_exhausted_callback exhausted_callback)
{
    exhausted_callback_ = lux::move(exhausted_callback);
}

void delayed_retry_executor::retry()
{
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

void delayed_retry_executor::reset()
{
    timer_->cancel();
    attempts_ = 0;
}

void delayed_retry_executor::on_timer_expired()
{
    ++attempts_;

    // If max_attempts is set to 0, it means infinite attempts are allowed.
    if (config_.max_attempts != 0 && attempts_ > config_.max_attempts)
    {
        if (exhausted_callback_)
        {
            exhausted_callback_();
        }
        return;
    }

    if (retry_action_)
    {
        retry_action_();
    }
}

std::chrono::milliseconds delayed_retry_executor::calculate_next_delay() const
{
    switch (config_.strategy)
    {
    case delayed_retry_config::backoff_strategy::fixed_delay:
        return std::min(config_.base_delay, config_.max_delay);
    case delayed_retry_config::backoff_strategy::linear_backoff:
        return calculate_linear_backoff_delay();
    case delayed_retry_config::backoff_strategy::exponential_backoff:
        return calculate_exponential_backoff_delay();
    }

    LUX_UNREACHABLE();
}

std::chrono::milliseconds delayed_retry_executor::calculate_linear_backoff_delay() const
{
    LUX_ASSERT(config_.strategy == delayed_retry_config::backoff_strategy::linear_backoff,
               "This method should only be called for linear backoff strategy");

    if (attempts_ == 0)
    {
        return std::min(config_.base_delay, config_.max_delay);
    }

    if (config_.base_delay.count() == 0)
    {
        return std::chrono::milliseconds::zero();
    }

    constexpr auto max_ms = static_cast<std::size_t>(std::chrono::milliseconds::max().count());
    const auto base_ms = static_cast<std::size_t>(config_.base_delay.count());

    // We ensure that the calculated delay does not exceed the maximum allowed delay.
    // It checks base_ms * attempts_ > max_ms but without potential overflow.
    if (base_ms > max_ms / attempts_)
    {
        return config_.max_delay;
    }

    const auto calculated_delay = std::chrono::milliseconds{base_ms * attempts_};
    const auto final_delay = std::min(calculated_delay, config_.max_delay);
    return final_delay;
}

std::chrono::milliseconds delayed_retry_executor::calculate_exponential_backoff_delay() const
{
    LUX_ASSERT(config_.strategy == delayed_retry_config::backoff_strategy::exponential_backoff,
               "This method should only be called for exponential backoff strategy");

    if (attempts_ == 0)
    {
        return std::min(config_.base_delay, config_.max_delay);
    }

    if (config_.base_delay.count() == 0)
    {
        return std::chrono::milliseconds::zero();
    }

    // Maximum bit shift for size_t (63 for 64-bit systems)
    constexpr auto max_bit_shift = sizeof(attempts_) * 8 - 1;

    const auto bit_shift = attempts_;
    if (bit_shift > max_bit_shift)
    {
        // Avoid overflow by capping the delay at max_delay
        return config_.max_delay;
    }

    const auto multiplier = std::size_t{1} << bit_shift; // 2 ^ (attempts_ - 1)
    constexpr auto max_ms = static_cast<std::size_t>(std::chrono::milliseconds::max().count());
    const auto base_ms = static_cast<std::size_t>(config_.base_delay.count());

    // We ensure that the calculated delay does not exceed the maximum allowed delay.
    // It checks base_ms * multiplier > max_ms but without potential overflow.
    if (base_ms > max_ms / multiplier)
    {
        return config_.max_delay;
    }

    const auto calculated_delay = std::chrono::milliseconds{base_ms * multiplier};
    const auto final_delay = std::min(calculated_delay, config_.max_delay);
    return final_delay;
}

} // namespace lux::time