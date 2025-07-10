#pragma once

#include <lux/fwd.hpp>

namespace lux {

class logger_factory
{
public:
    virtual lux::logger& get_logger(const char* name) = 0;

protected:
    virtual ~logger_factory() = default;
};

} // namespace lux