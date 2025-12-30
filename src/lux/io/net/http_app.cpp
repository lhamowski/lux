#include <lux/io/net/http_app.hpp>
#include <lux/io/net/base/http_factory.hpp>

#include <lux/support/assert.hpp>

namespace lux::net {

http_app::http_app(const lux::net::http_app_config& config, lux::net::base::http_factory& factory)
    : config_{config}, server_ptr_(factory.create_http_server(config_.server_config, *this))
{
}

http_app::http_app(const lux::net::http_app_config& config,
                   lux::net::base::http_factory& factory,
                   lux::net::base::ssl_context& ssl_context)
    : config_{config}, server_ptr_(factory.create_https_server(config_.server_config, *this, ssl_context))
{
}

http_app::~http_app()
{
    LUX_ASSERT(server_ptr_, "HTTP server pointer must not be null");
    server_ptr_->stop();
}

std::error_code http_app::serve(const lux::net::base::endpoint& ep)
{
    LUX_ASSERT(server_ptr_, "HTTP server pointer must not be null");
    return server_ptr_->serve(ep);
}

std::error_code http_app::stop()
{
    LUX_ASSERT(server_ptr_, "HTTP server pointer must not be null");
    return server_ptr_->stop();
}

void http_app::get(std::string_view target, handler_type handler)
{
    router_.add_route(lux::net::base::http_method::get, target, lux::move(handler));
}

void http_app::post(std::string_view target, handler_type handler)
{
    router_.add_route(lux::net::base::http_method::post, target, lux::move(handler));
}

void http_app::put(std::string_view target, handler_type handler)
{
    router_.add_route(lux::net::base::http_method::put, target, lux::move(handler));
}

void http_app::del(std::string_view target, handler_type handler)
{
    router_.add_route(lux::net::base::http_method::delete_, target, lux::move(handler));
}

void http_app::set_on_error_handler(error_handler_type handler)
{
    on_error_handler_ = lux::move(handler);
}

void http_app::on_server_started()
{
    // No-op
}

void http_app::on_server_stopped()
{
    // No-op
}

void http_app::on_server_error(const std::error_code& ec)
{
    if (on_error_handler_)
    {
        on_error_handler_(ec);
    }
}

lux::net::base::http_response http_app::handle_request(const lux::net::base::http_request& request)
{
    lux::net::base::http_response response;

    // Fill the common response fields
    response.set_version(request.version());
    response.set_header("Server", config_.server_name);

    router_.route(request, response);
    return response;
}

} // namespace lux::net