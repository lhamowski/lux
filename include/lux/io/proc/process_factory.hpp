#pragma once

#include <lux/io/proc/base/process_factory.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <string>

namespace lux::proc {

class process_factory : public lux::proc::base::process_factory
{
public:
    process_factory(boost::asio::any_io_executor executor);

    process_factory(const process_factory&) = delete;
    process_factory& operator=(const process_factory&) = delete;
    process_factory(process_factory&&) = default;
    process_factory& operator=(process_factory&&) = default;

public:
    lux::proc::base::process_ptr create_process(const std::string& executable_path,
                                                lux::proc::base::process_handler& handler) override;

private:
    boost::asio::any_io_executor executor_;
};

} // namespace lux::proc