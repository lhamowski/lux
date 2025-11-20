#pragma once

#include <lux/support/macros.hpp>

#include <functional>
#include <utility>

namespace lux {
namespace detail {

template <typename F>
class finally_guard
{
public:
    explicit finally_guard(F&& f) : func_(std::forward<F>(f))
    {
    }

    ~finally_guard()
    {
        func_();
    }

    finally_guard(const finally_guard&) = delete;
    finally_guard& operator=(const finally_guard&) = delete;
    finally_guard(finally_guard&&) = delete;
    finally_guard& operator=(finally_guard&&) = delete;

private:
    F func_;
};

template <typename F>
finally_guard<F> make_finally(F&& f)
{
    return finally_guard<F>(std::forward<F>(f));
}

} // namespace detail
} // namespace lux

#define LUX_FINALLY(code) [[maybe_unused]] auto LUX_UNIQUE_NAME(finally_) = ::lux::detail::make_finally([&]() { code; })