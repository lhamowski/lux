#pragma once

#include <lux/support/hash.hpp>

#include <unordered_map>
#include <string>
#include <functional>

namespace lux {

/**
 * A hash map with std::string keys using lux::string_hash for hashing
 * and std::equal_to<> for equality comparison.
 *
 * This map supports heterogeneous lookup, allowing lookups with
 * std::string_view and const char* without needing to construct
 * a std::string.
 *
 * @tparam T The type of the mapped values.
 */
template <typename T>
using string_unordered_map = std::unordered_map<std::string, T, lux::string_hash, std::equal_to<>>;

} // namespace lux