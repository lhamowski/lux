#pragma once

#include <expected>
#include <string>

namespace lux {

/**
 * @brief A result type that encapsulates either a successful value or an error message.
 *
 * The lux::result type is an alias for std::expected that uses std::string to represent error messages.
 * It can be used to indicate success or failure of operations in a clear and type-safe manner.
 *
 * @tparam T The type of the successful value. Defaults to void for operations that do not return a value.
 */
template <typename T = void>
using result = std::expected<T, std::string>;

}