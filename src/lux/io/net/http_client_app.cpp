#include <lux/io/net/http_client_app.hpp>
#include <lux/io/net/base/http_factory.hpp>

#include <lux/support/move.hpp>

namespace lux::net {

http_client_app::http_client_app(const lux::net::base::hostname_endpoint& destination,
                                 lux::net::base::http_factory& factory,
                                 const lux::net::http_client_app_config& config)
    : client_ptr_{factory.create_http_client(destination, config.client_config)}
{
}

http_client_app::http_client_app(const lux::net::base::hostname_endpoint& destination,
                                 lux::net::base::http_factory& factory,
                                 lux::net::base::ssl_context& ssl_context,
                                 const lux::net::http_client_app_config& config)
    : client_ptr_{factory.create_https_client(destination, config.client_config, ssl_context)}
{
}

void http_client_app::get(std::string_view target, handler_type handler, headers_type headers)
{
    lux::net::base::http_request request;
    request.set_method(lux::net::base::http_method::get);
    request.set_target(std::string{target});
    request.set_headers(headers);

    client_ptr_->request(request, lux::move(handler));
}

void http_client_app::post(std::string_view target, handler_type handler, headers_type headers, const std::string& body)
{
    lux::net::base::http_request request;
    request.set_method(lux::net::base::http_method::post);
    request.set_target(std::string{target});
    request.set_body(body);
    request.set_headers(lux::move(headers));

    client_ptr_->request(request, lux::move(handler));
}

void http_client_app::put(std::string_view target, handler_type handler, headers_type headers, const std::string& body)
{
    lux::net::base::http_request request;
    request.set_method(lux::net::base::http_method::put);
    request.set_target(std::string{target});
    request.set_body(body);
    request.set_headers(lux::move(headers));

    client_ptr_->request(request, lux::move(handler));
}

void http_client_app::del(std::string_view target, handler_type handler, headers_type headers, const std::string& body)
{
    lux::net::base::http_request request;
    request.set_method(lux::net::base::http_method::delete_);
    request.set_target(std::string{target});
    request.set_body(body);
    request.set_headers(lux::move(headers));

    client_ptr_->request(request, lux::move(handler));
}

} // namespace lux::net