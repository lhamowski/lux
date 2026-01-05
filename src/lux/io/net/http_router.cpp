#include <lux/io/net/http_router.hpp>
#include <lux/io/net/base/http_status.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/move.hpp>

#include <boost/url/parse.hpp>

namespace lux::net {

void http_router::add_route(lux::net::base::http_method method, std::string_view target, handler_type handler)
{
    route_key key{method, std::string{target}};

    LUX_ASSERT(!routes_.contains(key), "Route for method and target already exists");
    routes_[key] = lux::move(handler);
}

void http_router::route(const lux::net::base::http_request& request, lux::net::base::http_response& response) const
{
    const auto result = boost::urls::parse_origin_form(request.target());
    if (!result)
    {
        response.set_status(lux::net::base::http_status::bad_request);
        response.set_body("400 Bad Request");
        return;
    }

    route_key key{request.method(), std::string{result->segments().buffer()}};

    auto it = routes_.find(key);
    if (it != routes_.end())
    {
        it->second(request, response);
    }
    else
    {
        response.set_status(lux::net::base::http_status::not_found);
        response.set_body("404 Not Found");
    }
}

} // namespace lux::net