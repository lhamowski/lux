#pragma once

#include <magic_enum/magic_enum.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace lux {

template <typename T>
concept enumeration = std::is_enum_v<std::decay_t<T>>;

template <enumeration Enum>
constexpr auto to_underlying(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

template <enumeration Enum>
constexpr std::string_view to_string_view(Enum e) noexcept
{
    static_assert(magic_enum::is_magic_enum_supported, "magic_enum is not supported by this compiler");
    return magic_enum::enum_name(e);
}

} // namespace lux