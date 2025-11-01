#pragma once

#include <lux/support/move.hpp>

#include <ranges>
#include <string>
#include <vector>

namespace lux {

/**
 * @brief A utility class for building and joining strings.
 *
 * The `string_chain` class allows you to append and prepend strings,
 * and then join them into a single string with a specified delimiter.
 */
class string_chain
{
public:
    string_chain() = default;
    string_chain(std::string str)
    {
        append(lux::move(str));
    }

    string_chain(const string_chain&) = default;
    string_chain& operator=(const string_chain&) = default;
    string_chain(string_chain&&) = default;
    string_chain& operator=(string_chain&&) = default;

public:
    string_chain& append(std::string str)
    {
        strings_.emplace_back(lux::move(str));
        return *this;
    }

    string_chain& prepend(std::string str)
    {
        strings_.insert(strings_.begin(), lux::move(str));
        return *this;
    }

public:
    std::string join(const std::string& delimiter = "\n") const
    {
        return std::ranges::to<std::string>(strings_ | std::views::join_with(delimiter));
    }

private:
    std::vector<std::string> strings_;
};

} // namespace lux