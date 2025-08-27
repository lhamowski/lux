#pragma once

#include <concepts>
#include <type_traits>

namespace lux {

/**
 * @brief Concept to check if a type is trivially copyable for binary serialization.
 */
template <typename T>
concept trivially_copyable = std::is_trivially_copyable_v<T>;

/**
 * @brief Concept to check if a type is trivially copyable and default constructible.
 */
template <typename T>
concept trivially_readable = trivially_copyable<T> && std::default_initializable<T>;

/**
 * @brief Concept to check if a type is an rvalue reference.
 */
template <typename T>
concept rvalue = std::is_rvalue_reference_v<T&&>;

} // namespace lux