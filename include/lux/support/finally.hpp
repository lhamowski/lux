#pragma once

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

#define LUX_CONCAT_IMPL(x, y) x##y
#define LUX_CONCAT(x, y) LUX_CONCAT_IMPL(x, y)

#define LUX_FINALLY(code) auto LUX_CONCAT(finally_, __LINE__) = ::lux::detail::make_finally([&]() { code; })