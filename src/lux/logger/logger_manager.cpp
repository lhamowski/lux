#include <lux/logger/logger_manager.hpp>

#include <lux/logger/logger.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/overload.hpp>
#include <lux/support/move.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <algorithm>
#include <memory>

namespace lux {

logger_manager::logger_manager(const log_config& config)
{
    configure_sinks(config);
}

logger_manager::~logger_manager()
{
    spdlog::shutdown();
}

logger& logger_manager::get_logger(const char* name)
{
    LUX_ASSERT(name, "Logger name must not be null");

    auto it = loggers_.find(name);
    if (it != loggers_.end())
    {
        return it->second;
    }

    // Create a new logger with the configured sinks
    auto spd_logger = std::make_shared<spdlog::logger>(name, sinks_.begin(), sinks_.end());

    // Set the logger's level to the minimum level of all sinks so it doesn't log more than necessary.
    spd_logger->set_level(min_log_level_);

    // return reference to the new logger
    return loggers_.emplace(name, spd_logger).first->second;
}

void logger_manager::configure_sinks(const log_config& config)
{
    if (config.console)
    {
        configure_console_sink(*config.console);
    }

    if (config.file)
    {
        configure_file_sink(*config.file);
    }

    if (config.ostream)
    {
        configure_ostream_sink(*config.ostream);
    }

    // Set the minimum log level across all sinks
    if (!sinks_.empty())
    {
        const auto elem = std::ranges::min_element(sinks_, [](const auto& a, const auto& b) {
            return a->level() < b->level();
        });
        if (elem == sinks_.end())
        {
            min_log_level_ = spdlog::level::off;
            return;
        }

        min_log_level_ = (*elem)->level();
    }
}

void logger_manager::configure_console_sink(const console_log_config& config)
{
    using spdlog::sinks::stdout_color_sink_mt;

    auto console_sink = std::make_shared<stdout_color_sink_mt>();
    console_sink->set_pattern(config.pattern);
    console_sink->set_level(detail::to_spdlog_level(config.level));
    console_sink->set_color_mode(config.colorize ? spdlog::color_mode::automatic : spdlog::color_mode::never);
    sinks_.push_back(lux::move(console_sink));
}

void logger_manager::configure_ostream_sink(const ostream_log_config& config)
{
    using spdlog::sinks::ostream_sink_mt;

    auto stream_sink = std::make_shared<ostream_sink_mt>(config.stream.get(), config.force_flush);
    stream_sink->set_pattern(config.pattern);
    stream_sink->set_level(detail::to_spdlog_level(config.level));
    sinks_.push_back(lux::move(stream_sink));
}

void logger_manager::configure_file_sink(const file_log_config& config)
{
    std::visit(lux::overload{[this](const basic_file_log_config& cfg) { configure_basic_file_sink(cfg); },
                             [this](const rotating_file_log_config& cfg) { configure_rotating_file_sink(cfg); },
                             [this](const daily_file_log_config& cfg) { configure_daily_file_sink(cfg); }},
               config);
}

void logger_manager::configure_basic_file_sink(const basic_file_log_config& config)
{
    using spdlog::sinks::basic_file_sink_mt;

    auto file_sink = std::make_shared<basic_file_sink_mt>(config.filename);
    file_sink->set_pattern(config.pattern);
    file_sink->set_level(detail::to_spdlog_level(config.level));
    sinks_.push_back(lux::move(file_sink));
}

void logger_manager::configure_rotating_file_sink(const rotating_file_log_config& config)
{
    using spdlog::sinks::rotating_file_sink_mt;

    auto rotating_sink = std::make_shared<rotating_file_sink_mt>(config.filename, config.max_size, config.max_files);
    rotating_sink->set_pattern(config.pattern);
    rotating_sink->set_level(detail::to_spdlog_level(config.level));
    sinks_.push_back(lux::move(rotating_sink));
}

void logger_manager::configure_daily_file_sink(const daily_file_log_config& config)
{
    using spdlog::sinks::daily_file_sink_mt;

    auto daily_sink = std::make_shared<daily_file_sink_mt>(config.filename,
                                                           config.rotation_hour,
                                                           config.rotation_minute);
    daily_sink->set_pattern(config.pattern);
    daily_sink->set_level(detail::to_spdlog_level(config.level));
    sinks_.push_back(lux::move(daily_sink));
}

} // namespace lux
