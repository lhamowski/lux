#include <lux/crypto/detail/openssl_utils.hpp>

#include <openssl/err.h>

namespace lux::crypto::detail {

std::string get_openssl_error()
{
    const unsigned long err{ERR_get_error()};
    if (err == 0)
    {
        return "Unknown OpenSSL error";
    }

    char buffer[256]{};
    ERR_error_string_n(err, buffer, sizeof(buffer));
    return std::string{buffer};
}

} // namespace lux::crypto::detail