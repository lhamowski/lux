#pragma once

#include <lux/support/move.hpp>

#include <expected>
#include <format>
#include <string>
#include <utility>
#include <type_traits>

namespace lux {

/**
 * @brief A class representing an error message that can be built incrementally.
 */
class error_message
{
public:
    error_message() = default;
    error_message(std::string str)
    {
        append(lux::move(str));
    }

public:
    error_message& append(std::string str)
    {
        errors_.append(std::format("{}\n", str));
        return *this;
    }

    error_message& prepend(std::string str)
    {
        errors_ = std::format("{}\n{}", str, errors_);
        return *this;
    }

    const std::string& str() const
    {
        return errors_;
    }

    operator std::string() const
    {
        return errors_;
    }

private:
    std::string errors_;
};

/**
 * @brief A result type representing either a successful value of type T or an error message.
 * @tparam T The type of the successful value.
 */
template <typename T = void>
using result = std::expected<T, lux::error_message>;

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
    return std::unexpected<lux::error_message>{std::format(fmt, std::forward<Args>(args)...)};
}

inline constexpr auto err(std::string message)
{
    return std::unexpected<lux::error_message>{lux::move(message)};
}

} // namespace lux