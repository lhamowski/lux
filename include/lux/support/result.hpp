#pragma once

#include <lux/support/move.hpp>

#include <expected>
#include <format>
#include <string>
#include <utility>
#include <type_traits>

namespace lux {

/**
 * @brief A result type that encapsulates either a successful value or an error message.
 *
 * The lux::result type is an alias for std::expected that uses std::string to represent error messages.
 * It can be used to indicate success or failure of operations in a clear and type-safe manner.
 *
 * @tparam T The type of the successful value. Defaults to void for operations that do not return a value.
 */
template <typename T = void>
using result = std::expected<T, std::string>;

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
    return std::unexpected<std::string>{std::format(fmt, std::forward<Args>(args)...)};
}

inline constexpr auto err(std::string message)
{
    return std::unexpected<std::string>{lux::move(message)};
}

} // namespace lux