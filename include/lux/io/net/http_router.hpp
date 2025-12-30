#pragma once

#include <lux/io/net/base/http_method.hpp>
#include <lux/io/net/base/http_request.hpp>
#include <lux/io/net/base/http_response.hpp>

#include <boost/container_hash/hash.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace lux::net {

// TODO: Add path parameters, query parameters, middleware support, etc.
// For now, this is a simple router matching exact paths and methods.
class http_router
{
public:
    using handler_type = std::function<void(const lux::net::base::http_request&, lux::net::base::http_response&)>;

public:
    /**
     * Registers a handler for a specific HTTP method and target.
     * @param method The HTTP method to handle (e.g., GET, POST).
     * @param target The request target (path) to handle.
     * @param handler The function to handle the request and generate a response.
     */
    void add_route(lux::net::base::http_method method, std::string_view target, handler_type handler);

    /**
     * Routes an incoming HTTP request to the appropriate handler.
     * @param request The incoming HTTP request.
     * @param response The HTTP response to be populated by the handler.
     */
    void route(const lux::net::base::http_request& request, lux::net::base::http_response& response) const;

private:
    struct route_key
    {
        lux::net::base::http_method method;
        std::string target;

        bool operator==(const route_key&) const noexcept = default;
    };

    struct route_key_hash
    {
        std::size_t operator()(const route_key& key) const noexcept
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, key.method);
            boost::hash_combine(seed, key.target);
            return seed;
        }
    };

    std::unordered_map<route_key, handler_type, route_key_hash> routes_;
};

} // namespace lux::net