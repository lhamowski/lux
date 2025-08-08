#pragma once

#include <lux/io/time/base/timer.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/move.hpp>

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <functional>

namespace lux::time {

template <typename Clock>
class basic_interval_timer : public lux::time::base::interval_timer
{
public:
    using timer_type = boost::asio::basic_waitable_timer<Clock>;

public:
    basic_interval_timer(boost::asio::any_io_executor executor) : timer_{executor}
    {
    }

public:
    // lux::time::base::interval_timer implementation
    void set_handler(std::function<void()> callback) override
    {
        if (handler_)
        {
            LUX_ASSERT(false, "Handler for timer is already set.");
            return;
        }

        handler_ = lux::move(callback);
    }

    void schedule(std::chrono::milliseconds delay) override
    {
        canceled_ = false;
        next_deadline_ = Clock::now() + delay;

        timer_.expires_at(next_deadline_);
        timer_.async_wait([this](const boost::system::error_code& ec) {
            if (ec)
            {
                // Be careful here - object might be destroyed before this callback is called.
                return;
            }

            if (handler_)
            {
                handler_();
            }
        });
    }

    void schedule_periodic(std::chrono::milliseconds interval) override
    {
        canceled_ = false;
        interval_ = interval;
        next_deadline_ = Clock::now() + interval;
        schedule_next();
    }

    void cancel() override
    {
        canceled_ = true;
        timer_.cancel();
    }

private:
    void schedule_next()
    {
        timer_.expires_at(next_deadline_);
        timer_.async_wait([this](const boost::system::error_code& ec) {
            if (ec)
            {
                // Be careful here - object might be destroyed before this callback is called.
                return;
            }

            if (canceled_)
            {
                return; // If the timer was canceled, we should not execute the handler.
            }

            if (handler_)
            {
                handler_();
            }

            if (canceled_)
            {
                return; // The time could have been canceled in the handler, so we should not reschedule.
            }

            next_deadline_ += interval_;
            schedule_next();
        });
    }

private:
    timer_type timer_;
    std::function<void()> handler_{};
    std::chrono::milliseconds interval_{};
    std::chrono::steady_clock::time_point next_deadline_{};

    bool canceled_{false};
};

using interval_timer = basic_interval_timer<std::chrono::steady_clock>;

} // namespace lux::time