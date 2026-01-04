#pragma once

#include <cstddef>
#include <random>
#include <vector>

namespace lux {

/**
 * @brief Generate a vector of random bytes using a non-deterministic random number generator.
 * @param count The number of random bytes to generate.
 * @return A vector containing the generated random bytes.
 */
[[nodiscard]] std::vector<std::byte> random_bytes(std::size_t count)
{

    if (count == 0)
    {
        return {};
    }

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, 255);

    std::vector<std::byte> result;
    result.resize(count);

    for (std::size_t i = 0; i < count; ++i)
    {
        result[i] = static_cast<std::byte>(dist(rng));
    }

    return result;
}

} // namespace lux