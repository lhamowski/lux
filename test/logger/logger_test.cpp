#include "test_case.hpp"

#include <lux/logger/logger.hpp>
#include <lux/logger/logger_manager.hpp>

#include <lux/support/finally.hpp>

#include <catch2/catch_all.hpp>

#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/details/os.h>
#include <spdlog/logger.h>

#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>

LUX_TEST_CASE("logger", "logs messages at various levels with formatting", "[logger]")
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
        spd_logger->set_level(spdlog::level::trace); // Allow all levels
        spd_logger->set_pattern("%l: %v");           // Simple pattern for testing

        lux::logger logger{spd_logger};

        const char* message = "Test message";
        logger.log(lux::log_level::info, message);
        spd_logger->flush();

        const std::string output = oss.str();
        CHECK(output.find("info: Test message") != std::string::npos);
    }

    SECTION("Logger logs formatted messages with consteval format string")
    {
        std::ostringstream oss;
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
        auto spd_logger = std::make_shared<spdlog::logger>("test_logger", sink);
        spd_logger->set_level(spdlog::level::trace);
        spd_logger->set_pattern("%v");

        lux::logger logger{spd_logger};

        logger.log(lux::log_level::info, "Number: {}, String: {}", 42, "test");
        spd_logger->flush();

        const std::string output = oss.str();
        CHECK(output.find("Number: 42, String: test") != std::string::npos);
    }

    SECTION("Logger logs formatted messages with runtime format std::string")
    {
        std::ostringstream oss;
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
        auto spd_logger = std::make_shared<spdlog::logger>("test_logger", sink);
        spd_logger->set_level(spdlog::level::trace);
        spd_logger->set_pattern("%v");

        lux::logger logger{spd_logger};

        std::string format_string = "Number: {}, String: {}";
        logger.log(lux::log_level::info, lux::runtime(format_string), 42, "test");

        std::string msg = "Test string";
        logger.log(lux::log_level::info, msg);

        std::string msg2 = "Lorem ipsum";
        logger.log(lux::log_level::critical, lux::move(msg2));

        spd_logger->flush();

        const std::string output = oss.str();
        CHECK(output.find("Number: 42, String: test") != std::string::npos);
        CHECK(output.find("Test string") != std::string::npos);
        CHECK(output.find("Lorem ipsum") != std::string::npos);
    }

    SECTION("Logger logs formatted messages with runtime format std::string_view")
    {
        std::ostringstream oss;
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
        auto spd_logger = std::make_shared<spdlog::logger>("test_logger", sink);
        spd_logger->set_level(spdlog::level::trace);
        spd_logger->set_pattern("%v");

        lux::logger logger{spd_logger};

        std::string str = "Number: {}, String: {}";
        std::string_view format_string = str;
        logger.log(lux::log_level::info, lux::runtime(format_string), 42, "test");
        spd_logger->flush();

        const std::string output = oss.str();
        CHECK(output.find("Number: 42, String: test") != std::string::npos);
    }

    SECTION("Logger automatically converts enum arguments to string")
    {
        std::ostringstream oss;
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
        auto spd_logger = std::make_shared<spdlog::logger>("test_logger", sink);
        spd_logger->set_level(spdlog::level::trace);
        spd_logger->set_pattern("%v");

        lux::logger logger{spd_logger};

        enum class color
        {
            red,
            green,
            blue
        };

        logger.log(lux::log_level::info, "Color: {}", color::green);
        spd_logger->flush();

        const std::string output = oss.str();
        CHECK(output.find("Color: green") != std::string::npos);
    }

    SECTION("Logger supports std::span arguments")
    {
        std::ostringstream oss;
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
        auto spd_logger = std::make_shared<spdlog::logger>("test_logger", sink);
        spd_logger->set_level(spdlog::level::trace);
        spd_logger->set_pattern("%v");

        lux::logger logger{spd_logger};
        std::array<std::byte, 3> bytes{std::byte{0x01}, std::byte{0x02}, std::byte{0xFF}};
        std::span bytes_view{bytes};

        logger.log(lux::log_level::info, "Bytes: {:X}", lux::to_hex(bytes_view));
        spd_logger->flush();

        const std::string output = oss.str();
        CHECK(output.find("Bytes:") != std::string::npos);
        CHECK(output.find("0000: 01 02 FF") != std::string::npos);
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

        const std::string output = oss.str();
        CHECK(output.find("trace") != std::string::npos);
        CHECK(output.find("debug") != std::string::npos);
        CHECK(output.find("info") != std::string::npos);
        CHECK(output.find("warning") != std::string::npos);
        CHECK(output.find("error") != std::string::npos);
        CHECK(output.find("critical") != std::string::npos);
    }
}

LUX_TEST_CASE("logger_manager", "creates loggers with various sink configurations", "[logger_manager]")
{
    SECTION("Logger manager can be created with console config")
    {
        lux::log_config config;
        config.console = lux::console_log_config{};

        REQUIRE_NOTHROW(lux::logger_manager{config});
    }

    SECTION("Logger manager can be created with basic file config")
    {
        lux::log_config config;
        config.file = lux::basic_file_log_config{.level = lux::log_level::info,
                                                 .filename = "test_log.log",
                                                 .pattern = "[%Y-%m-%d %H:%M:%S] %v"};

        // Ensure the file does not exist before creating the logger manager
        std::filesystem::remove("test_log.log");

        // finally block to clean up the file after the test
        LUX_FINALLY({ std::filesystem::remove("test_log.log"); });

        REQUIRE_NOTHROW(lux::logger_manager{config});

        // Check if the file was created
        CHECK(std::filesystem::exists("test_log.log"));
    }

    SECTION("Logger manager can be created with rotating file config")
    {
        lux::log_config config;
        lux::rotating_file_log_config rotating_config;
        rotating_config.level = lux::log_level::info;
        rotating_config.filename = "rotating_test.log";
        rotating_config.max_size = 1024 * 1024;
        rotating_config.max_files = 3;
        config.file = rotating_config;

        // Ensure the file does not exist before creating the logger manager
        std::filesystem::remove("rotating_test.log");

        // finally block to clean up the file after the test
        LUX_FINALLY({ std::filesystem::remove("rotating_test.log"); });

        REQUIRE_NOTHROW(lux::logger_manager{config});

        // Check if the file was created
        CHECK(std::filesystem::exists("rotating_test.log"));
    }

    SECTION("Logger manager can be created with daily file config")
    {
        lux::log_config config;
        lux::daily_file_log_config daily_config;
        daily_config.level = lux::log_level::warn;
        daily_config.filename = "daily_test.log";
        daily_config.rotation_hour = 23;   // Rotate at 11 PM
        daily_config.rotation_minute = 59; // Rotate at 59 minutes past the hour
        config.file = daily_config;

        const auto tm = spdlog::details::os::localtime();
        const auto expected_filename = std::format("daily_test_{:04d}-{:02d}-{:02d}.log",
                                                   tm.tm_year + 1900,
                                                   tm.tm_mon + 1,
                                                   tm.tm_mday);

        // Ensure the file does not exist before creating the logger manager
        std::filesystem::remove(expected_filename);

        // finally block to clean up the file after the test
        LUX_FINALLY({ std::filesystem::remove(expected_filename); });

        REQUIRE_NOTHROW(lux::logger_manager{config});

        // Check if the file was created with the current date in the filename
        CHECK(std::filesystem::exists(expected_filename));
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
}

LUX_TEST_CASE("log_config", "provides default values for all configuration types", "[log_config]")
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
        CHECK(config.max_size == 10 * 1024 * 1024); // 10MB
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

LUX_TEST_CASE("logger", "logs messages end-to-end with console and file sinks", "[logger][integration]")
{
    SECTION("Complete logging workflow")
    {
        // Create a logger manager with console output
        lux::log_config config;

        lux::console_log_config console_config;
        console_config.level = lux::log_level::debug;
        console_config.pattern = "[%l] %n: %v";

        lux::basic_file_log_config file_config;
        file_config.level = lux::log_level::info;
        file_config.filename = "integration_test.log";
        file_config.pattern = "[%Y-%m-%d %H:%M:%S] [%^%l%$] [%n] %v";

        config.console = console_config;
        config.file = file_config;

        // Ensure the file does not exist before creating the logger manager
        std::filesystem::remove("integration_test.log");

        // finally block to clean up the file after the test
        LUX_FINALLY({ std::filesystem::remove("integration_test.log"); });

        std::optional<lux::logger_manager> manager;

        REQUIRE_NOTHROW(manager.emplace(config));
        REQUIRE(manager.has_value());

        // Check sinks
        const auto& sinks = manager->sinks();
        REQUIRE(sinks.size() == 2); // One console sink and one file sink

        CHECK(sinks[0]->level() == spdlog::level::debug);
        CHECK(sinks[1]->level() == spdlog::level::info);

        // Get a logger and use it
        auto& logger = manager->get_logger("test");

        // Log messages at different levels
        REQUIRE_NOTHROW(LUX_LOG_TRACE(logger, "Trace message"));
        REQUIRE_NOTHROW(LUX_LOG_DEBUG(logger, "Debug message with value: {}", 123));
        REQUIRE_NOTHROW(LUX_LOG_INFO(logger, "Starting integration test"));
        REQUIRE_NOTHROW(LUX_LOG_WARN(logger, "Warning message"));
        REQUIRE_NOTHROW(LUX_LOG_ERROR(logger, "Error message"));
        REQUIRE_NOTHROW(LUX_LOG_CRITICAL(logger, "Critical message"));

        // Flush the logger to ensure all messages are written
        REQUIRE_NOTHROW(logger.flush());

        // Check if the file was created and contains the expected log messages
        CHECK(std::filesystem::exists("integration_test.log"));
        std::ifstream log_file("integration_test.log");
        REQUIRE(log_file.is_open());

        // Debug log message is not in the file, as it is only logged to console.

        // Check info log message
        std::string line;
        std::getline(log_file, line);

        // Check expected pattern using regex
        std::regex info_pattern(
            R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\] \[info\] \[test\] Starting integration test)");
        CHECK(std::regex_search(line, info_pattern));

        // Check warning log message
        std::getline(log_file, line);
        std::regex warn_pattern(R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\] \[warning\] \[test\] Warning message)");
        CHECK(std::regex_search(line, warn_pattern));

        // Check error log message
        std::getline(log_file, line);
        std::regex error_pattern(R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\] \[error\] \[test\] Error message)");
        CHECK(std::regex_search(line, error_pattern));

        // Check critical log message
        std::getline(log_file, line);
        std::regex critical_pattern(
            R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\] \[critical\] \[test\] Critical message)");
        CHECK(std::regex_search(line, critical_pattern));
    }
}

