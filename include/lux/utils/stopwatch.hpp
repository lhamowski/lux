#pragma once

#include <chrono>

namespace lux {

template <typename Clock>
class basic_stopwatch
{
public:
    basic_stopwatch() : start_time_{Clock::now()} {}

public:
    void reset() { start_time_ = Clock::now(); }

    std::chrono::milliseconds elapsed_ms() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start_time_);
    }

    template <typename Duration>
    Duration elapsed() const
    {
        return std::chrono::duration_cast<Duration>(Clock::now() - start_time_);
    }

private:
    Clock::time_point start_time_;
};

using stopwatch = basic_stopwatch<std::chrono::steady_clock>;

} // namespace lux