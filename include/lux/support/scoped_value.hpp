#pragma once

#include <lux/support/macros.hpp>

namespace lux {
namespace detail {

template <typename T>
class scoped_value_guard
{
public:
    explicit scoped_value_guard(T& value, T scoped_value) : value_{value}, original_value_{value}
    {
        value_ = scoped_value;
    }

    ~scoped_value_guard()
    {
        value_ = original_value_;
    }

private:
    T& value_;
    T original_value_;
};

} // namespace detail
} // namespace lux

#define LUX_SCOPED_VALUE(var, value) [[maybe_unused]] auto LUX_UNIQUE_NAME(scoped_value_guard_) = ::lux::detail::scoped_value_guard{var, value}