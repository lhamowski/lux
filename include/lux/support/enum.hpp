#pragma once

#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 1024

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

    if (const auto name = magic_enum::enum_name(e); !name.empty())
    {
        return name;
    }

    if (const auto underlying_value = to_underlying(e); underlying_value < MAGIC_ENUM_RANGE_MIN)
    {
        return "<error:value is less than enum range minimum (0)>";
    }
    else if (underlying_value > MAGIC_ENUM_RANGE_MAX)
    {
        return "<error:value is greater than enum range maximum (1024)>";
    }

    return "<error:unknown>";
}

} // namespace lux