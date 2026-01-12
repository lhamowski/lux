#include "test_case.hpp"

#include <lux/io/proc/process.hpp>
#include <lux/io/proc/process_factory.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/io_context.hpp>

namespace {

class test_process_handler : public lux::proc::base::process_handler
{
public:
    void on_process_error(const std::string& /*error_message*/) override
    {
    }

    void on_process_exit(int /*exit_code*/) override
    {
    }

    void on_process_stdout(std::string_view /*out*/) override
    {
    }

    void on_process_stderr(std::string_view /*err*/) override
    {
    }
};

} // namespace

LUX_TEST_CASE("process_factory", "creates process successfully", "[io][process]")
{
    boost::asio::io_context io_context;

    test_process_handler handler;
    lux::proc::process_factory factory{io_context.get_executor()};

    auto process = factory.create_process("some_executable_path", handler);
    REQUIRE(process != nullptr);
}