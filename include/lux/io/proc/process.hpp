#pragma once

#include <lux/io/proc/base/process.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <memory>
#include <string>
#include <vector>

namespace lux::proc {

class process : public lux::proc::base::process
{
public:
    process(boost::asio::any_io_executor executor,
            lux::proc::base::process_handler& handler,
            const std::string& exe_path);

    ~process();

    process(const process&) = delete;
    process& operator=(const process&) = delete;
    process(process&&) = default;
    process& operator=(process&&) = default;

public:
    // lux::proc::base::process implementation
    lux::status start(const std::vector<std::string>& args) override;
    void terminate() override;
    bool is_running() const override;

private:
    class impl;
    std::shared_ptr<impl> impl_;
};

} // namespace lux::proc