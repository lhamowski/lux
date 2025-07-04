#include <lux/logger/logger.hpp>
#include <lux/logger/logger_manager.hpp>

#include <catch2/catch_all.hpp>

#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/logger.h>

#include <sstream>
#include <memory>
#include <iostream>

TEST_CASE("Logger basic functionality", "[logger]")
{
    SECTION("Logger can be created with spdlog logger")
    {
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(std::cout);
        auto spd_logger = std::make_shared<spdlog::logger>("test_logger", sink);
        
        REQUIRE_NOTHROW(lux::logger{spd_logger});
    }

    SECTION("Logger logs messages with different levels")
    {
        std::ostringstream oss;
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
        auto spd_logger = std::make_shared<spdlog::logger>("test_logger", sink);
        spd_logger->set_level(spdlog::level::trace);  // Allow all levels
        spd_logger->set_pattern("%l: %v");  // Simple pattern for testing
        
        lux::logger logger{spd_logger};
        
        logger.log(lux::log_level::info, "Test message");
        spd_logger->flush();
        
        std::string output = oss.str();
        CHECK(output.find("info: Test message") != std::string::npos);
    }

    SECTION("Logger logs formatted messages")
    {
        std::ostringstream oss;
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
        auto spd_logger = std::make_shared<spdlog::logger>("test_logger", sink);
        spd_logger->set_level(spdlog::level::trace);
        spd_logger->set_pattern("%v");
        
        lux::logger logger{spd_logger};
        
        logger.log(lux::log_level::info, "Number: {}, String: {}", 42, "test");
        spd_logger->flush();
        
        std::string output = oss.str();
        CHECK(output.find("Number: 42, String: test") != std::string::npos);
    }

    SECTION("Logger supports all log levels")
    {
        std::ostringstream oss;
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
        auto spd_logger = std::make_shared<spdlog::logger>("test_logger", sink);
        spd_logger->set_level(spdlog::level::trace);
        spd_logger->set_pattern("%l");
        
        lux::logger logger{spd_logger};
        
        logger.log(lux::log_level::trace, "trace");
        logger.log(lux::log_level::debug, "debug");
        logger.log(lux::log_level::info, "info");
        logger.log(lux::log_level::warn, "warn");
        logger.log(lux::log_level::error, "error");
        logger.log(lux::log_level::critical, "critical");
        spd_logger->flush();
        
        std::string output = oss.str();
        CHECK(output.find("trace") != std::string::npos);
        CHECK(output.find("debug") != std::string::npos);
        CHECK(output.find("info") != std::string::npos);
        CHECK(output.find("warning") != std::string::npos);
        CHECK(output.find("error") != std::string::npos);
        CHECK(output.find("critical") != std::string::npos);
    }
}

TEST_CASE("Logger manager basic functionality", "[logger_manager]")
{
    SECTION("Logger manager can be created with console config")
    {
        lux::log_config config;
        config.console = lux::console_log_config{};
        
        REQUIRE_NOTHROW(lux::logger_manager{config});
    }

    SECTION("Logger manager can be created with file config")
    {
        lux::log_config config;
        config.file = lux::basic_file_log_config{
            .level = lux::log_level::info,
            .filename = "test_log.log",
            .pattern = "[%Y-%m-%d %H:%M:%S] %v"
        };
        
        REQUIRE_NOTHROW(lux::logger_manager{config});
    }

    SECTION("Logger manager can be created with both console and file config")
    {
        lux::log_config config;
        config.console = lux::console_log_config{
            .level = lux::log_level::debug,
            .pattern = "[%H:%M:%S] %v",
            .colorize = false
        };

        lux::rotating_file_log_config rotating_config;
        rotating_config.level = lux::log_level::info;
        rotating_config.filename = "rotating_test.log";
        rotating_config.max_size = 1024 * 1024;
        rotating_config.max_files = 3;
        config.file = rotating_config;
        
        REQUIRE_NOTHROW(lux::logger_manager{config});
    }

    SECTION("Logger manager provides loggers by name")
    {
        lux::log_config config;
        config.console = lux::console_log_config{};
        
        lux::logger_manager manager{config};
        
        auto& logger1 = manager.get_logger("test_logger_1");
        auto& logger2 = manager.get_logger("test_logger_2");
        auto& logger1_again = manager.get_logger("test_logger_1");
        
        // Should return the same logger instance for the same name
        CHECK(&logger1 == &logger1_again);
        CHECK(&logger1 != &logger2);
    }

    SECTION("Logger manager supports daily file rotation config")
    {
        lux::log_config config;

        lux::daily_file_log_config daily_config;
        daily_config.level = lux::log_level::warn;
        daily_config.filename = "daily_test.log";
        daily_config.rotation_hour = 23;
        daily_config.rotation_minute = 59;
        config.file = daily_config;
        
        REQUIRE_NOTHROW(lux::logger_manager{config});
    }
}

TEST_CASE("Log configuration structures", "[log_config]")
{
    SECTION("Console log config has reasonable defaults")
    {
        lux::console_log_config config;
        
        CHECK(config.level == lux::log_level::info);
        CHECK(config.pattern == lux::default_log_pattern);
        CHECK(config.colorize == true);
    }

    SECTION("Basic file log config has reasonable defaults")
    {
        lux::basic_file_log_config config;
        
        CHECK(config.level == lux::log_level::info);
        CHECK(config.filename == "lux.log");
        CHECK(config.pattern == lux::default_log_pattern);
    }

    SECTION("Rotating file log config has reasonable defaults")
    {
        lux::rotating_file_log_config config;
        
        CHECK(config.level == lux::log_level::info);
        CHECK(config.filename == "lux.log");
        CHECK(config.max_size == 10 * 1024 * 1024);  // 10MB
        CHECK(config.max_files == 5);
    }

    SECTION("Daily file log config has reasonable defaults")
    {
        lux::daily_file_log_config config;
        
        CHECK(config.level == lux::log_level::info);
        CHECK(config.filename == "lux.log");
        CHECK(config.rotation_hour == 0);
        CHECK(config.rotation_minute == 0);
    }
}

TEST_CASE("Integration test - end to end logging", "[logger][integration]")
{
    SECTION("Complete logging workflow")
    {
        // Create a logger manager with console output
        lux::log_config config;
        config.console = lux::console_log_config{
            .level = lux::log_level::debug,
            .pattern = "[%l] %n: %v",
            .colorize = false
        };
        
        lux::logger_manager manager{config};
        
        // Get a logger and use it
        auto& logger = manager.get_logger("integration_test");
        
        // These should not throw
        REQUIRE_NOTHROW(logger.log(lux::log_level::info, "Starting integration test"));
        REQUIRE_NOTHROW(logger.log(lux::log_level::debug, "Debug message with value: {}", 123));
        REQUIRE_NOTHROW(logger.log(lux::log_level::warn, "Warning message"));
        REQUIRE_NOTHROW(logger.log(lux::log_level::error, "Error message"));
    }
}


