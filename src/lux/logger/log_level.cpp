#include <lux/logger/log_level.hpp>

#include <lux/support/assert.hpp>

namespace lux::detail {

spdlog::level::level_enum to_spdlog_level(log_level level)
{
    switch (level)
    {
    case log_level::trace:
        return spdlog::level::trace;
    case log_level::debug:
        return spdlog::level::debug;
    case log_level::info:
        return spdlog::level::info;
    case log_level::warn:
        return spdlog::level::warn;
    case log_level::error:
        return spdlog::level::err;
    case log_level::critical:
        return spdlog::level::critical;
    case log_level::none:
        return spdlog::level::off;
    }
    LUX_UNREACHABLE();
}

} // namespace lux::detail