#pragma once

#include <openssl/crypto.h>

#include <cstddef>
#include <vector>

namespace lux::crypto {

/**
 * Secure allocator using OpenSSL's secure memory functions.
 * Allocates memory using OPENSSL_secure_malloc and cleanses it before freeing.
 * This allocator is intended for use with sensitive data that requires secure handling.
 */
template <typename T>
struct secure_allocator
{
    using value_type = T;

    secure_allocator() = default;

    template <typename U>
    secure_allocator(const secure_allocator<U>&) noexcept
    {
    }

    T* allocate(std::size_t n)
    {
        return static_cast<T*>(OPENSSL_secure_malloc(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        if (p)
        {
            OPENSSL_cleanse(p, n * sizeof(T));
            OPENSSL_free(p);
        }
    }

    template <typename U>
    bool operator==(const secure_allocator<U>&) const noexcept
    {
        return true;
    }

    template <typename U>
    bool operator!=(const secure_allocator<U>&) const noexcept
    {
        return false;
    }
};

/**
 * Secure vector using the secure_allocator.
 * This vector type is suitable for storing sensitive data securely.
 */
template <typename T>
using secure_vector = std::vector<T, secure_allocator<T>>;

/**
 * Secure string using the secure_allocator.
 * This string type is suitable for storing sensitive string data securely.
 */
using secure_string = std::basic_string<char, std::char_traits<char>, secure_allocator<char>>;

} // namespace lux::crypto