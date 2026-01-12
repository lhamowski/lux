#pragma once

#include <lux/io/proc/base/process.hpp>

#include <string>

namespace lux::proc::base {

class process_factory
{
public:
    virtual lux::proc::base::process_ptr create_process(const std::string& executable_path,
                                                        lux::proc::base::process_handler& handler) = 0;

protected:
    virtual ~process_factory() = default;
};

} // namespace lux::proc::base