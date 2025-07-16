#pragma once

#include <type_traits>

namespace lux {

template <typename Enum>
constexpr auto to_underlying(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

} // namespace lux