#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace lux::proc::base {

class process_handler
{
public:
    virtual void on_process_error(const std::string& error_message) = 0;
    virtual void on_process_exit(int exit_code) = 0;
    virtual void on_process_stdout(std::string_view out) = 0;
    virtual void on_process_stderr(std::string_view err) = 0;

protected:
    virtual ~process_handler() = default;
};


class process
{
public:
    virtual ~process() = default;

public:
    virtual void start(const std::vector<std::string>& args) = 0;
    virtual void terminate() = 0;
    virtual bool is_running() const = 0;
};

using process_ptr = std::unique_ptr<process>;

} // namespace lux::proc::base