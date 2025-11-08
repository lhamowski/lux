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
    template <std::convertible_to<std::string> T>
    error_message& append(T&& str)
    {
        errors_.append(std::format("{}\n", std::forward<T>(str)));
        return *this;
    }

    template <std::convertible_to<std::string> T>
    error_message& prepend(T&& str)
    {
        errors_ = std::format("{}\n{}", std::forward<T>(str), errors_);
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

    bool empty() const
    {
        return errors_.empty();
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

namespace std {

template <>
struct formatter<lux::error_message>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const lux::error_message& msg, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", msg.str());
    }
};

} // namespace std
