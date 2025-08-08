#pragma once

#include "interval_timer_mock.hpp"

#include <lux/io/time/base/timer.hpp>

#include <vector>

namespace test {

class timer_factory_mock : public lux::time::base::timer_factory
{
public:
    // lux::time::base::timer_factory implementation
    lux::time::base::interval_timer_ptr create_interval_timer() override
    {
        auto timer = std::make_unique<interval_timer_mock>();
        created_timers_.push_back(timer.get());
        return timer;
    }

public:
    // Be careful with this - created timers are not owned by this factory.
    std::vector<interval_timer_mock*> created_timers_;
};

} // namespace test