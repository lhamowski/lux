#pragma once

#include <lux/logger/log_level.hpp>
#include <lux/logger/logger.hpp>

#include <spdlog/common.h>

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <unordered_map>

namespace lux {

static constexpr std::string_view default_log_pattern{"%Y-%m-%d %H:%M:%S.%e [%^%l%$] <%n> %v"};
static constexpr auto default_log_level{log_level::info};

struct console_log_config
{
    log_level level{default_log_level};
    std::string pattern{default_log_pattern}; 
    bool colorize{true};
};

struct basic_file_log_config
{
    log_level level{default_log_level};
    std::string filename{"lux.log"};
    std::string pattern{default_log_pattern};
};

struct rotating_file_log_config : public basic_file_log_config
{
    size_t max_size{10 * 1024 * 1024}; // 10 MB
    size_t max_files{5};               
};

struct daily_file_log_config : public basic_file_log_config
{
    // Rotate at midnight by default
    unsigned rotation_hour{0}; 
    unsigned rotation_minute{0};
};

using file_log_config = std::variant<basic_file_log_config, rotating_file_log_config, daily_file_log_config>;

struct log_config
{
    std::optional<console_log_config> console;
    std::optional<file_log_config> file;
};

class logger_manager final
{
public:
    logger_manager(const log_config& config);
    ~logger_manager();

    logger_manager(const logger_manager&) = delete;
    logger_manager& operator=(const logger_manager&) = delete;
    logger_manager(logger_manager&&) = default;
    logger_manager& operator=(logger_manager&&) = default;

public:
    // Get or create a logger by name
    logger& get_logger(const char* name);

    const auto& sinks() const { return sinks_; }
    const auto& loggers() const { return loggers_; }

private:
    void configure_sinks(const log_config& config);
    void configure_console_sink(const console_log_config& config);
    void configure_file_sink(const file_log_config& config);
    void configure_basic_file_sink(const basic_file_log_config& config);
    void configure_rotating_file_sink(const rotating_file_log_config& config);
    void configure_daily_file_sink(const daily_file_log_config& config);

private:
    std::vector<spdlog::sink_ptr> sinks_;
    std::unordered_map<std::string, logger> loggers_;

    // Minimum log level across all sinks to prevent unnecessary logging
    spdlog::level::level_enum min_log_level_{spdlog::level::off};
};

} // namespace lux
