#pragma once

#include <lux/logger/log_level.hpp>
#include <lux/support/assert.hpp>
#include <lux/support/move.hpp>

#include <spdlog/logger.h>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <concepts>
#include <memory>
#include <utility>
#include <string_view>

namespace lux {

// Concept for compile-time format strings
template<typename T>
concept compile_time_format_string = fmt::is_compiled_string<T>::value;

class logger final
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

   // Overload for compile-time format string validation
   template <compile_time_format_string FormatString, typename... Args>
   void log(log_level level, const FormatString& fmt, Args&&... args)
   {
       spd_logger_->log(detail::to_spdlog_level(level), fmt, std::forward<Args>(args)...);
   }

   // Overload for string_view (runtime validation)
   template <typename... Args>
   void log(log_level level, std::string_view fmt, Args&&... args)
   {
       spd_logger_->log(detail::to_spdlog_level(level), fmt::runtime(fmt), std::forward<Args>(args)...);
   }

private:
   std::shared_ptr<spdlog::logger> spd_logger_;
};

} // namespace lux