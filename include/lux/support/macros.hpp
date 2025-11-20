#pragma once

/**
 * @brief Concatenate two tokens.
 */
#define LUX_CONCAT_IMPL(x, y) x##y
#define LUX_CONCAT(x, y) LUX_CONCAT_IMPL(x, y)

/**
 * @brief Generate a unique name by concatenating the base name with the current line number and counter.
 */
#define LUX_UNIQUE_NAME(base) LUX_CONCAT(base, LUX_CONCAT(__LINE__, __COUNTER__))
