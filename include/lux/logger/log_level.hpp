#pragma once

#include <spdlog/common.h>

namespace lux {

enum class log_level
{
    trace,
    debug,
    info,
    warn,
    error,
    critical,
    none
};

namespace detail {

spdlog::level::level_enum to_spdlog_level(log_level level);

}

} // namespace lux