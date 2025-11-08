#pragma once

#include <exception>
#include <format>
#include <string>
#include <string_view>

namespace lux {

class formatted_exception : public std::exception
{
public:
    template <typename... Args>
    formatted_exception(const char* msg, const Args&... args) : msg_{std::vformat(msg, std::make_format_args(args...))}
    {
    }

    explicit formatted_exception(const char* msg) : msg_{msg}
    {
    }

public:
    const char* what() const noexcept override
    {
        return msg_.c_str();
    }

private:
    std::string msg_;
};

} // namespace lux