#include <lux/io/net/http_client.hpp>

#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/net/base/http_request.hpp>
#include <lux/io/net/base/http_response.hpp>
#include <lux/io/net/base/http_status.hpp>
#include <lux/io/net/base/socket_factory.hpp>
#include <lux/io/net/base/tcp_socket.hpp>

#include <lux/io/net/detail/http_parser.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/move.hpp>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstring>
#include <functional>
#include <queue>
#include <string>
#include <utility>

namespace lux::net {

namespace {

using boost_http_request_type = detail::boost_http_request_type;

detail::boost_http_request_type from_lux_http_request(lux::net::base::http_request&& request)
{
    using boost_verb_type = boost::beast::http::verb;
    using lux_method_type = lux::net::base::http_method;

    static const std::unordered_map<lux::net::base::http_method, boost_verb_type> method_map = {
        {lux_method_type::get, boost_verb_type::get},
        {lux_method_type::post, boost_verb_type::post},
        {lux_method_type::put, boost_verb_type::put},
        {lux_method_type::delete_, boost_verb_type::delete_},
        {lux_method_type::unknown, boost_verb_type::unknown}};

    boost_http_request_type boost_request;

    const auto method_it = method_map.find(request.method());
    boost_request.method(method_it != method_map.end() ? method_it->second : boost_verb_type::unknown);
    boost_request.target(request.target());
    boost_request.version(request.version());

    for (const auto& [key, value] : request.headers())
    {
        boost_request.set(key, value);
    }

    boost_request.body() = request.body();
    boost_request.prepare_payload();

    return boost_request;
}

lux::net::base::http_response from_boost_http_response(detail::boost_http_response_type&& boost_response)
{
    lux::net::base::http_response response;

    using boost_status_type = boost::beast::http::status;
    using lux_status_type = lux::net::base::http_status;

    static const std::unordered_map<boost_status_type, lux::net::base::http_status> status_map = {
        {boost_status_type::unknown, lux_status_type::unknown},

        {boost_status_type::continue_, lux_status_type::continue_},
        {boost_status_type::switching_protocols, lux_status_type::switching_protocols},
        {boost_status_type::processing, lux_status_type::processing},
        {boost_status_type::early_hints, lux_status_type::early_hints},

        {boost_status_type::ok, lux_status_type::ok},
        {boost_status_type::created, lux_status_type::created},
        {boost_status_type::accepted, lux_status_type::accepted},
        {boost_status_type::non_authoritative_information, lux_status_type::non_authoritative_information},
        {boost_status_type::no_content, lux_status_type::no_content},
        {boost_status_type::reset_content, lux_status_type::reset_content},
        {boost_status_type::partial_content, lux_status_type::partial_content},
        {boost_status_type::multi_status, lux_status_type::multi_status},
        {boost_status_type::already_reported, lux_status_type::already_reported},
        {boost_status_type::im_used, lux_status_type::im_used},

        {boost_status_type::multiple_choices, lux_status_type::multiple_choices},
        {boost_status_type::moved_permanently, lux_status_type::moved_permanently},
        {boost_status_type::found, lux_status_type::found},
        {boost_status_type::see_other, lux_status_type::see_other},
        {boost_status_type::not_modified, lux_status_type::not_modified},
        {boost_status_type::use_proxy, lux_status_type::use_proxy},
        {boost_status_type::temporary_redirect, lux_status_type::temporary_redirect},
        {boost_status_type::permanent_redirect, lux_status_type::permanent_redirect},

        {boost_status_type::bad_request, lux_status_type::bad_request},
        {boost_status_type::unauthorized, lux_status_type::unauthorized},
        {boost_status_type::payment_required, lux_status_type::payment_required},
        {boost_status_type::forbidden, lux_status_type::forbidden},
        {boost_status_type::not_found, lux_status_type::not_found},
        {boost_status_type::method_not_allowed, lux_status_type::method_not_allowed},
        {boost_status_type::not_acceptable, lux_status_type::not_acceptable},
        {boost_status_type::proxy_authentication_required, lux_status_type::proxy_authentication_required},
        {boost_status_type::request_timeout, lux_status_type::request_timeout},
        {boost_status_type::conflict, lux_status_type::conflict},
        {boost_status_type::gone, lux_status_type::gone},
        {boost_status_type::length_required, lux_status_type::length_required},
        {boost_status_type::precondition_failed, lux_status_type::precondition_failed},
        {boost_status_type::payload_too_large, lux_status_type::payload_too_large},
        {boost_status_type::uri_too_long, lux_status_type::uri_too_long},
        {boost_status_type::unsupported_media_type, lux_status_type::unsupported_media_type},
        {boost_status_type::range_not_satisfiable, lux_status_type::range_not_satisfiable},
        {boost_status_type::expectation_failed, lux_status_type::expectation_failed},
        {boost_status_type::misdirected_request, lux_status_type::misdirected_request},
        {boost_status_type::unprocessable_entity, lux_status_type::unprocessable_entity},
        {boost_status_type::locked, lux_status_type::locked},
        {boost_status_type::failed_dependency, lux_status_type::failed_dependency},
        {boost_status_type::too_early, lux_status_type::too_early},
        {boost_status_type::upgrade_required, lux_status_type::upgrade_required},
        {boost_status_type::precondition_required, lux_status_type::precondition_required},
        {boost_status_type::too_many_requests, lux_status_type::too_many_requests},
        {boost_status_type::request_header_fields_too_large, lux_status_type::request_header_fields_too_large},
        {boost_status_type::unavailable_for_legal_reasons, lux_status_type::unavailable_for_legal_reasons},

        {boost_status_type::internal_server_error, lux_status_type::internal_server_error},
        {boost_status_type::not_implemented, lux_status_type::not_implemented},
        {boost_status_type::bad_gateway, lux_status_type::bad_gateway},
        {boost_status_type::service_unavailable, lux_status_type::service_unavailable},
        {boost_status_type::gateway_timeout, lux_status_type::gateway_timeout},
        {boost_status_type::http_version_not_supported, lux_status_type::http_version_not_supported},
        {boost_status_type::variant_also_negotiates, lux_status_type::variant_also_negotiates},
        {boost_status_type::insufficient_storage, lux_status_type::insufficient_storage},
        {boost_status_type::loop_detected, lux_status_type::loop_detected},
        {boost_status_type::not_extended, lux_status_type::not_extended},
        {boost_status_type::network_authentication_required, lux_status_type::network_authentication_required}};

    const auto status_it = status_map.find(boost_response.result());
    response.set_status(status_it != status_map.end() ? status_it->second : lux_status_type::unknown);
    response.set_version(boost_response.version());

    for (auto& field : boost_response)
    {
        response.set_header(field.name_string(), field.value());
    }

    response.set_body(lux::move(boost_response.body()));
    return response;
}

class client_response_parser_handler : public detail::http_response_parser_handler
{
public:
    virtual void on_response_parsed(const lux::net::base::http_response& response) = 0;

public:
    void on_message_parsed(detail::boost_http_response_type&& message) override final
    {
        on_response_parsed(from_boost_http_response(lux::move(message)));
    }
};

// Helper to create TCP socket config from HTTP client config
lux::net::base::tcp_socket_config create_tcp_config(const lux::net::base::http_client_config& http_config)
{
    lux::net::base::tcp_socket_config tcp_config;
    tcp_config.keep_alive = http_config.keep_alive;
    tcp_config.buffer = http_config.buffer;
    tcp_config.reconnect.enabled = false; // HTTP client does not auto-reconnect
    return tcp_config;
}

} // namespace

class http_client::impl : public lux::net::base::tcp_socket_handler, public client_response_parser_handler
{
public:
    impl(const lux::net::base::hostname_endpoint& destination,
         const lux::net::base::http_client_config& config,
         lux::net::base::socket_factory& socket_factory)
        : socket_{socket_factory.create_tcp_socket(create_tcp_config(config), *this)},
          destination_{destination},
          parser_{*this}
    {
    }

    impl(const lux::net::base::hostname_endpoint& destination,
         const lux::net::base::http_client_config& config,
         lux::net::base::socket_factory& socket_factory,
         lux::net::base::ssl_context& ssl_context)
        : socket_{socket_factory.create_ssl_tcp_socket(create_tcp_config(config), ssl_context, *this)},
          destination_{destination},
          parser_{*this}
    {
    }

    ~impl()
    {
        if (socket_)
        {
            socket_->disconnect(true);
        }
    }

private:
    struct pending_request
    {
        lux::net::base::http_request request;
        lux::net::base::http_client_handler_type handler;
    };

public:
    void request(const lux::net::base::http_request& request, lux::net::base::http_client_handler_type handler)
    {
        LUX_ASSERT(socket_, "Socket must be initialized");
        LUX_ASSERT(handler, "Handler must be valid");

        // Enqueue request
        request_queue_.push({request, lux::move(handler)});

        // Process next request if no active request
        if (!current_request_.has_value())
        {
            process_next_request();
        }
    }

private:
    void process_next_request()
    {
        LUX_ASSERT(!request_queue_.empty(), "Request queue must not be empty");

        current_request_ = lux::move(request_queue_.front());
        request_queue_.pop();

        if (!socket_->is_connected())
        {
            // Initiate connection - request will be sent in on_connected
            const auto connect_error = socket_->connect(destination_);
            if (connect_error)
            {
                // Connection error - notify callback and process next
                notify_current_request_error(connect_error);
                current_request_.reset();
                process_next_request();
            }
        }
        else
        {
            // Socket already connected - send immediately
            send_current_request();
        }
    }

    void send_current_request()
    {
        LUX_ASSERT(current_request_.has_value(), "There must be a current request to send");

        auto boost_request = from_lux_http_request(lux::move(current_request_->request));

        using serializer_type = boost::beast::http::request_serializer<boost::beast::http::string_body>;
        auto serializer = serializer_type{lux::move(boost_request)};

        boost::system::error_code ec;
        do
        {
            serializer.next(ec, [&](boost::beast::error_code& serializer_ec, const auto& buf_seq) {
                for (const auto& buf : buf_seq)
                {
                    const auto buf_view = std::span{reinterpret_cast<const std::byte*>(buf.data()), buf.size()};
                    if (const auto err = socket_->send(buf_view); err)
                    {
                        serializer_ec = err;
                    }
                    serializer.consume(buf.size());
                }
            });
        } while (!ec && !serializer.is_done());

        if (ec)
        {
            notify_current_request_error(ec);
            current_request_.reset();

            if (!request_queue_.empty())
            {
                process_next_request();
            }
        }
    }

    void notify_current_request_error(const std::error_code& ec)
    {
        LUX_ASSERT(current_request_.has_value(), "There must be a current request to notify");
        LUX_ASSERT(ec, "Error code must indicate an error");

        current_request_->handler(std::unexpected{ec});
    }

    void notify_current_request_success(const lux::net::base::http_response& response)
    {
        LUX_ASSERT(current_request_.has_value(), "There must be a current request to notify");

        current_request_->handler(response);
    }

private:
    // lux::net::base::tcp_socket_handler implementation
    void on_connected(lux::net::base::tcp_socket& socket) override
    {
        std::ignore = socket;

        LUX_ASSERT(current_request_.has_value(), "There must be a current request after connection");
        send_current_request();
    }

    void on_disconnected(lux::net::base::tcp_socket& socket, const std::error_code& ec, bool will_reconnect) override
    {
        LUX_ASSERT(!will_reconnect, "HTTP client does not support automatic reconnection");

        std::ignore = socket;
        std::ignore = will_reconnect;

        if (!ec)
        {
            // Normal disconnection
            return;
        }

        // Connection error - always notify current request immediately
        // Even if reconnect is scheduled, this specific request has failed
        if (current_request_.has_value())
        {
            notify_current_request_error(ec);
            current_request_.reset();
        }

        if (!request_queue_.empty())
        {
            process_next_request();
        }
    }

    void on_data_read(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) override
    {
        std::ignore = socket;
        parser_.parse(data);
    }

    void on_data_sent(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) override
    {
        std::ignore = socket;
        std::ignore = data;
        // No action needed - waiting for response in on_data_read
    }

private:
    // http_response_parser_handler implementation
    void on_response_parsed(const lux::net::base::http_response& response) override
    {
        if (!current_request_.has_value())
        {
            return; // No active request, ignore response
        }

        notify_current_request_success(response);
        current_request_.reset();

        if (!request_queue_.empty())
        {
            process_next_request();
        }
        else
        {
            // No pending requests - disconnect socket
            socket_->disconnect(true);
        }
    }

    void on_parse_error(const std::error_code& ec) override
    {
        if (!current_request_.has_value())
        {
            return; // No active request, ignore error
        }

        notify_current_request_error(ec);
        current_request_.reset();

        if (!request_queue_.empty())
        {
            process_next_request();
        }
        else
        {
            // No pending requests - disconnect socket
            socket_->disconnect(true);
        }
    }

private:
    lux::net::base::tcp_socket_ptr socket_{nullptr};
    lux::net::base::hostname_endpoint destination_;

    std::queue<pending_request> request_queue_;
    std::optional<pending_request> current_request_;
    detail::http_response_parser parser_;
};

http_client::http_client(const lux::net::base::hostname_endpoint& destination,
                         const lux::net::base::http_client_config& config,
                         lux::net::base::socket_factory& socket_factory)
    : impl_{std::make_unique<impl>(destination, config, socket_factory)}
{
}

http_client::http_client(const lux::net::base::hostname_endpoint& destination,
                         const lux::net::base::http_client_config& config,
                         lux::net::base::socket_factory& socket_factory,
                         lux::net::base::ssl_context& ssl_context)
    : impl_{std::make_unique<impl>(destination, config, socket_factory, ssl_context)}
{
}

http_client::~http_client() = default;

void http_client::request(const lux::net::base::http_request& request, lux::net::base::http_client_handler_type handler)
{
    impl_->request(request, lux::move(handler));
}

} // namespace lux::net