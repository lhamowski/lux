#pragma once

#include <lux/logger/log_level.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/enum.hpp>
#include <lux/support/move.hpp>
#include <lux/support/result.hpp>

#include <spdlog/logger.h>
#include <spdlog/fmt/bin_to_hex.h>

#include <fmt/ranges.h>

#include <format>
#include <memory>
#include <ranges>
#include <string_view>

namespace lux {

inline auto runtime(std::string_view s)
{
    return fmt::runtime(s);
}

template <typename T>
auto to_hex(const T& value)
{
    return spdlog::to_hex(value);
}

class logger
{
public:
    explicit logger(std::shared_ptr<spdlog::logger> spd_logger) : spd_logger_(lux::move(spd_logger))
    {
        LUX_ASSERT(spd_logger_, "spdlog::logger must not be null");
    }

    logger(const logger&) = delete;
    logger& operator=(const logger&) = delete;
    logger(logger&&) = default;
    logger& operator=(logger&&) = default;

private:
    template <typename T>
    static auto preprocess_argument(T&& arg)
    {
        return std::forward<T>(arg);
    }

    template <lux::enumeration T>
    static auto preprocess_argument(T&& arg)
    {
        return lux::to_string_view(arg);
    }

    template <typename T>
    using preprocessed_argument_t = decltype(preprocess_argument(std::declval<T>()));

public:
    template <typename... Args>
    void log(log_level level, fmt::format_string<preprocessed_argument_t<Args>...> fmt, Args&&... args)
    {
        spd_logger_->log(detail::to_spdlog_level(level), fmt, preprocess_argument(std::forward<Args>(args))...);
    }

    void flush() { spd_logger_->flush(); }

private:
    std::shared_ptr<spdlog::logger> spd_logger_;
};

} // namespace lux

#define LUX_LOG(logger, level, ...) (logger).log(level, __VA_ARGS__)
#define LUX_LOG_TRACE(logger, ...) LUX_LOG(logger, lux::log_level::trace, __VA_ARGS__)
#define LUX_LOG_DEBUG(logger, ...) LUX_LOG(logger, lux::log_level::debug, __VA_ARGS__)
#define LUX_LOG_INFO(logger, ...) LUX_LOG(logger, lux::log_level::info, __VA_ARGS__)
#define LUX_LOG_WARN(logger, ...) LUX_LOG(logger, lux::log_level::warn, __VA_ARGS__)
#define LUX_LOG_ERROR(logger, ...) LUX_LOG(logger, lux::log_level::error, __VA_ARGS__)
#define LUX_LOG_CRITICAL(logger, ...) LUX_LOG(logger, lux::log_level::critical, __VA_ARGS__)

namespace lux {

/**
 * @brief Creates an error result and logs it at ERROR level.
 *
 * @tparam T The result type
 * @tparam Args Format arguments types
 * @param logger Logger instance to log the error
 * @param fmt Format string
 * @param args Format arguments
 * @return lux::result<T> Error result with formatted message
 */
template <typename T, typename... Args>
[[nodiscard]] lux::result<T> log_err(logger& logger_, std::string_view fmt, Args&&... args)
{
    auto error_msg = std::vformat(fmt, std::make_format_args(args...));
    LUX_LOG_ERROR(logger_, "{}", error_msg);
    return std::unexpected<std::string>{lux::move(error_msg)};
}

/**
* @brief Creates an error status and logs it at ERROR level.
*
* Convenience alias for log_err<void>
*/
template <typename... Args>
[[nodiscard]] lux::status log_err(logger& logger, std::string_view fmt, Args&&... args)
{
    return log_err<void>(logger, fmt, std::forward<Args>(args)...);
}

} // namespace lux