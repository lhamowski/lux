#pragma once

#include <utility>

namespace lux {

// A utility function to move a value while ensuring it is not const.
constexpr auto move(auto&& value) noexcept
{
    using raw_type = typename std::remove_reference<decltype(value)>::type;
    static_assert(!std::is_const_v<raw_type>, "move should not be called on const lvalues");

    return std::move(value);
}

} // namespace lux
