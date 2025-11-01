#pragma once

#include <lux/support/move.hpp>
#include <lux/utils/string_chain.hpp>

#include <expected>
#include <format>
#include <string>
#include <utility>
#include <type_traits>

namespace lux {

/**
 * @brief A result type representing either a successful value of type T or an error message (lux::string_chain).
 * @tparam T The type of the successful value.
 */
template <typename T = void>
using result = std::expected<T, lux::string_chain>;

/**
 * @brief A result type specialized for void, representing success or failure without a value.
 */
using status = result<>;

template <typename T = void>
inline constexpr auto ok(T&& value)
{
    return result<std::decay_t<T>>{std::forward<T>(value)};
}

inline constexpr auto ok()
{
    return status{};
}

template <typename... Args>
inline auto err(std::format_string<Args...> fmt, Args&&... args)
{
    return std::unexpected<lux::string_chain>{std::format(fmt, std::forward<Args>(args)...)};
}

inline constexpr auto err(std::string message)
{
    return std::unexpected<lux::string_chain>{lux::move(message)};
}

} // namespace lux