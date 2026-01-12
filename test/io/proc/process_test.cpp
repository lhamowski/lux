#include "test_case.hpp"

#include <lux/io/proc/process.hpp>

#include <boost/asio/io_context.hpp>

#include <catch2/catch_all.hpp>

#include <chrono>
#include <string>
#include <vector>
#include <filesystem>

namespace {

// Path to the test helper executable (built alongside tests)
#ifdef _WIN32
constexpr const char* test_helper_path = "process-test-helper.exe";
#else
constexpr const char* test_helper_path = "process-test-helper";
#endif

class test_process_handler : public lux::proc::base::process_handler
{
public:
    void on_process_error(const std::string& error_message) override
    {
        errors.emplace_back(error_message);
    }

    void on_process_exit(int code) override
    {
        this->exit_code = code;
        exit_called = true;
    }

    void on_process_stdout(std::string_view out) override
    {
        stdout_data += out;
    }

    void on_process_stderr(std::string_view err) override
    {
        stderr_data += err;
    }

    bool exit_called{false};
    int exit_code{-1};
    std::string stdout_data;
    std::string stderr_data;
    std::vector<std::string> errors;
};

void run_io_context_until(boost::asio::io_context& io_ctx, auto predicate, std::chrono::milliseconds timeout)
{
    const auto start = std::chrono::steady_clock::now();
    while (!predicate())
    {
        io_ctx.poll_one();
        if (std::chrono::steady_clock::now() - start > timeout)
        {
            break;
        }
    }
}

} // namespace

LUX_TEST_CASE("process", "captures stdout from child process", "[io][proc]")
{
    boost::asio::io_context io_ctx;
    test_process_handler handler;

    lux::proc::process proc{io_ctx.get_executor(), handler, test_helper_path};
    proc.start({"stdout"});

    run_io_context_until(io_ctx, [&] { return handler.exit_called; }, std::chrono::seconds{5});

    REQUIRE(handler.exit_called);
    CHECK(handler.exit_code == 0);
    CHECK(handler.stdout_data == "test stdout message");
    CHECK(handler.stderr_data.empty());
}

LUX_TEST_CASE("process", "captures stderr from child process", "[io][proc]")
{
    boost::asio::io_context io_ctx;
    test_process_handler handler;

    lux::proc::process proc{io_ctx.get_executor(), handler, test_helper_path};
    proc.start({"stderr"});

    run_io_context_until(io_ctx, [&] { return handler.exit_called; }, std::chrono::seconds{5});

    REQUIRE(handler.exit_called);
    CHECK(handler.exit_code == 0);
    CHECK(handler.stdout_data.empty());
    CHECK(handler.stderr_data == "test stderr message");
}

LUX_TEST_CASE("process", "captures both stdout and stderr from child process", "[io][proc]")
{
    boost::asio::io_context io_ctx;
    test_process_handler handler;

    lux::proc::process proc{io_ctx.get_executor(), handler, test_helper_path};
    proc.start({"both"});

    run_io_context_until(io_ctx, [&] { return handler.exit_called; }, std::chrono::seconds{5});

    REQUIRE(handler.exit_called);
    CHECK(handler.exit_code == 0);
    CHECK(handler.stdout_data == "stdout line");
    CHECK(handler.stderr_data == "stderr line");
}

LUX_TEST_CASE("process", "returns custom exit code from child process", "[io][proc]")
{
    boost::asio::io_context io_ctx;
    test_process_handler handler;

    lux::proc::process proc{io_ctx.get_executor(), handler, test_helper_path};
    proc.start({"exit_code", "42"});

    run_io_context_until(io_ctx, [&] { return handler.exit_called; }, std::chrono::seconds{5});

    REQUIRE(handler.exit_called);
    CHECK(handler.exit_code == 42);
}

LUX_TEST_CASE("process", "returns zero exit code for successful process", "[io][proc]")
{
    boost::asio::io_context io_ctx;
    test_process_handler handler;

    lux::proc::process proc{io_ctx.get_executor(), handler, test_helper_path};
    proc.start({"exit_code", "0"});

    run_io_context_until(io_ctx, [&] { return handler.exit_called; }, std::chrono::seconds{5});

    REQUIRE(handler.exit_called);
    CHECK(handler.exit_code == 0);
}

LUX_TEST_CASE("process", "reports running state correctly", "[io][proc]")
{
    boost::asio::io_context io_ctx;
    test_process_handler handler;

    lux::proc::process proc{io_ctx.get_executor(), handler, test_helper_path};

    CHECK_FALSE(proc.is_running());

    proc.start({"sleep", "1"});

    // Process should be running initially
    io_ctx.poll();
    CHECK(proc.is_running());

    // Terminate and wait for exit
    proc.terminate();
    run_io_context_until(io_ctx, [&] { return handler.exit_called; }, std::chrono::seconds{5});

    CHECK_FALSE(proc.is_running());
}

LUX_TEST_CASE("process", "terminates running child process", "[io][proc]")
{
    boost::asio::io_context io_ctx;
    test_process_handler handler;

    lux::proc::process proc{io_ctx.get_executor(), handler, test_helper_path};
    proc.start({"sleep", "10"});

    io_ctx.poll();
    REQUIRE(proc.is_running());

    proc.terminate();
    run_io_context_until(io_ctx, [&] { return handler.exit_called; }, std::chrono::seconds{5});

    REQUIRE(handler.exit_called);
    CHECK(handler.exit_code != 0); // Terminated process should not have exit code 0
    CHECK_FALSE(proc.is_running());
}
