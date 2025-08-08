#pragma once

#include <lux/io/time/base/timer.hpp>
#include <lux/io/time/interval_timer.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <chrono>
#include <memory>

namespace lux::time {

template <typename Clock>
class basic_timer_factory : public lux::time::base::timer_factory
{
public:
    basic_timer_factory(boost::asio::any_io_executor executor) : executor_{executor}
    {
    }

public:
    // lux::time::base::timer_factory implementation
    lux::time::base::interval_timer_ptr create_interval_timer() override
    {
        return std::make_unique<basic_interval_timer<Clock>>(executor_);
    }

private:
    boost::asio::any_io_executor executor_;
};

using timer_factory = basic_timer_factory<std::chrono::steady_clock>;

} // namespace lux::time