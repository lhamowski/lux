#pragma once

namespace lux::net::base {

enum class http_method
{
    unknown,
    get,
    post,
    put,
    delete_,
    // ... Add more methods as needed ...
    unsupported,
};

} // namespace lux::net::base