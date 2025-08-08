#pragma once

#include <lux/io/time/base/timer.hpp>

#include <chrono>
#include <cstddef>
#include <functional>

namespace test {

class interval_timer_mock : public lux::time::base::interval_timer
{
public:
    // lux::time::base::interval_timer implementation
    void set_handler(std::function<void()> callback) override
    {
        ++set_handler_call_count_;
        handler_ = std::move(callback);
    }

    void schedule(std::chrono::milliseconds delay) override
    {
        ++schedule_call_count_;
        scheduled_delay_ = delay;
    }

    void schedule_periodic(std::chrono::milliseconds interval) override
    {
        ++schedule_periodic_call_count_;
        periodic_interval_ = interval;
    }

    void cancel() override
    {
        ++cancel_call_count_;
        canceled_ = true;
    }

public:
    void execute_handler()
    {
        if (handler_)
        {
            handler_();
        }
    }

public:
    std::chrono::milliseconds scheduled_delay() const
    {
        return scheduled_delay_;
    }

    std::chrono::milliseconds periodic_interval() const
    {
        return periodic_interval_;
    }

    bool canceled() const
    {
        return canceled_;
    }

public:
    std::size_t set_handler_call_count() const
    {
        return set_handler_call_count_;
    }

    std::size_t schedule_call_count() const
    {
        return schedule_call_count_;
    }

    std::size_t schedule_periodic_call_count() const
    {
        return schedule_periodic_call_count_;
    }

    std::size_t cancel_call_count() const
    {
        return cancel_call_count_;
    }

private:
    std::function<void()> handler_;
    std::chrono::milliseconds scheduled_delay_{};
    std::chrono::milliseconds periodic_interval_{};
    bool canceled_{false};

private:
    std::size_t set_handler_call_count_{0};
    std::size_t schedule_call_count_{0};
    std::size_t schedule_periodic_call_count_{0};
    std::size_t cancel_call_count_{0};
};

} // namespace test