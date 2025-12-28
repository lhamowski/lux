#include <lux/io/net/http_server.hpp>

#include <lux/io/net/base/http_request.hpp>
#include <lux/io/net/base/http_response.hpp>
#include <lux/io/net/base/http_status.hpp>
#include <lux/io/net/base/socket_factory.hpp>
#include <lux/io/net/base/tcp_acceptor.hpp>
#include <lux/io/net/base/tcp_socket.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/move.hpp>
#include <lux/utils/expiring_ref.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/string_body.hpp>

#include <atomic>
#include <cstring>
#include <functional>
#include <optional>

namespace lux::net {

namespace {

using boost_http_request_type = boost::beast::http::request<boost::beast::http::string_body>;
using boost_http_response_type = boost::beast::http::response<boost::beast::http::string_body>;

lux::net::base::http_request from_boost_http_request(boost_http_request_type&& boost_request)
{
    lux::net::base::http_request request;

    switch (boost_request.method())
    {
    case boost::beast::http::verb::get:
        request.set_method(lux::net::base::http_method::get);
        break;
    case boost::beast::http::verb::post:
        request.set_method(lux::net::base::http_method::post);
        break;
    case boost::beast::http::verb::put:
        request.set_method(lux::net::base::http_method::put);
        break;
    case boost::beast::http::verb::delete_:
        request.set_method(lux::net::base::http_method::delete_);
        break;
    case boost::beast::http::verb::unknown:
        request.set_method(lux::net::base::http_method::unknown);
        break;
    default:
        // Unsupported method
        request.set_method(lux::net::base::http_method::unsupported);
        break;
    }

    request.set_target(boost_request.target());
    request.set_version(boost_request.version());

    for (auto& field : boost_request)
    {
        request.set_header(field.name_string(), field.value());
    }

    request.set_body(lux::move(boost_request.body()));
    return request;
}

class http_parser_handler
{
public:
    virtual void on_request_parsed(const lux::net::base::http_request& request) = 0;
    virtual void on_parse_error(const std::error_code& ec) = 0;

protected:
    virtual ~http_parser_handler() = default;
};

class http_parser
{
public:
    explicit http_parser(http_parser_handler& handler) : handler_{handler}
    {
        parser_.emplace();
    }

public:
    void parse(const std::span<const std::byte>& data)
    {
        LUX_ASSERT(parser_, "Parser must be initialized at this point");

        const auto buf = buffer_.prepare(data.size());
        std::memcpy(buf.data(), data.data(), data.size());
        buffer_.commit(data.size());

        while (buffer_.size() > 0)
        {
            boost::beast::error_code ec;
            const std::size_t bytes_consumed = parser_->put(buffer_.data(), ec);

            if (ec == boost::beast::http::error::need_more)
            {
                // Need more data to complete the parsing
                buffer_.consume(bytes_consumed);
                return;
            }
            else if (ec)
            {
                // Parsing error occurred
                buffer_.consume(buffer_.size()); // Clear the buffer
                parser_.emplace();               // Reset the parser
                handler_.on_parse_error(ec);
                return;
            }

            if (parser_->is_done())
            {
                // Successfully parsed a complete HTTP request
                lux::net::base::http_request request = from_boost_http_request(parser_->release());
                parser_.emplace(); // Reset the parser for the next request
                handler_.on_request_parsed(request);
                buffer_.consume(bytes_consumed);
            }
            else
            {
                buffer_.consume(bytes_consumed);
            }
        }
    }

private:
    http_parser_handler& handler_;

private:
    boost::beast::flat_buffer buffer_;

    using parser_type = boost::beast::http::request_parser<boost::beast::http::string_body>;
    std::optional<parser_type> parser_;
};

boost_http_response_type from_lux_http_response(lux::net::base::http_response&& response)
{
    using boost_type = boost::beast::http::status;

    static const std::unordered_map<lux::net::base::http_status, boost_type> status_map = {
        {lux::net::base::http_status::unknown, boost_type::unknown},

        {lux::net::base::http_status::continue_, boost_type::continue_},
        {lux::net::base::http_status::switching_protocols, boost_type::switching_protocols},
        {lux::net::base::http_status::processing, boost_type::processing},
        {lux::net::base::http_status::early_hints, boost_type::early_hints},

        {lux::net::base::http_status::ok, boost_type::ok},
        {lux::net::base::http_status::created, boost_type::created},
        {lux::net::base::http_status::accepted, boost_type::accepted},
        {lux::net::base::http_status::non_authoritative_information, boost_type::non_authoritative_information},
        {lux::net::base::http_status::no_content, boost_type::no_content},
        {lux::net::base::http_status::reset_content, boost_type::reset_content},
        {lux::net::base::http_status::partial_content, boost_type::partial_content},
        {lux::net::base::http_status::multi_status, boost_type::multi_status},
        {lux::net::base::http_status::already_reported, boost_type::already_reported},
        {lux::net::base::http_status::im_used, boost_type::im_used},

        {lux::net::base::http_status::multiple_choices, boost_type::multiple_choices},
        {lux::net::base::http_status::moved_permanently, boost_type::moved_permanently},
        {lux::net::base::http_status::found, boost_type::found},
        {lux::net::base::http_status::see_other, boost_type::see_other},
        {lux::net::base::http_status::not_modified, boost_type::not_modified},
        {lux::net::base::http_status::use_proxy, boost_type::use_proxy},
        {lux::net::base::http_status::temporary_redirect, boost_type::temporary_redirect},
        {lux::net::base::http_status::permanent_redirect, boost_type::permanent_redirect},

        {lux::net::base::http_status::bad_request, boost_type::bad_request},
        {lux::net::base::http_status::unauthorized, boost_type::unauthorized},
        {lux::net::base::http_status::payment_required, boost_type::payment_required},
        {lux::net::base::http_status::forbidden, boost_type::forbidden},
        {lux::net::base::http_status::not_found, boost_type::not_found},
        {lux::net::base::http_status::method_not_allowed, boost_type::method_not_allowed},
        {lux::net::base::http_status::not_acceptable, boost_type::not_acceptable},
        {lux::net::base::http_status::proxy_authentication_required, boost_type::proxy_authentication_required},
        {lux::net::base::http_status::request_timeout, boost_type::request_timeout},
        {lux::net::base::http_status::conflict, boost_type::conflict},
        {lux::net::base::http_status::gone, boost_type::gone},
        {lux::net::base::http_status::length_required, boost_type::length_required},
        {lux::net::base::http_status::precondition_failed, boost_type::precondition_failed},
        {lux::net::base::http_status::payload_too_large, boost_type::payload_too_large},
        {lux::net::base::http_status::uri_too_long, boost_type::uri_too_long},
        {lux::net::base::http_status::unsupported_media_type, boost_type::unsupported_media_type},
        {lux::net::base::http_status::range_not_satisfiable, boost_type::range_not_satisfiable},
        {lux::net::base::http_status::expectation_failed, boost_type::expectation_failed},
        {lux::net::base::http_status::misdirected_request, boost_type::misdirected_request},
        {lux::net::base::http_status::unprocessable_entity, boost_type::unprocessable_entity},
        {lux::net::base::http_status::locked, boost_type::locked},
        {lux::net::base::http_status::failed_dependency, boost_type::failed_dependency},
        {lux::net::base::http_status::too_early, boost_type::too_early},
        {lux::net::base::http_status::upgrade_required, boost_type::upgrade_required},
        {lux::net::base::http_status::precondition_required, boost_type::precondition_required},
        {lux::net::base::http_status::too_many_requests, boost_type::too_many_requests},
        {lux::net::base::http_status::request_header_fields_too_large, boost_type::request_header_fields_too_large},
        {lux::net::base::http_status::unavailable_for_legal_reasons, boost_type::unavailable_for_legal_reasons},

        {lux::net::base::http_status::internal_server_error, boost_type::internal_server_error},
        {lux::net::base::http_status::not_implemented, boost_type::not_implemented},
        {lux::net::base::http_status::bad_gateway, boost_type::bad_gateway},
        {lux::net::base::http_status::service_unavailable, boost_type::service_unavailable},
        {lux::net::base::http_status::gateway_timeout, boost_type::gateway_timeout},
        {lux::net::base::http_status::http_version_not_supported, boost_type::http_version_not_supported},
        {lux::net::base::http_status::variant_also_negotiates, boost_type::variant_also_negotiates},
        {lux::net::base::http_status::insufficient_storage, boost_type::insufficient_storage},
        {lux::net::base::http_status::loop_detected, boost_type::loop_detected},
        {lux::net::base::http_status::not_extended, boost_type::not_extended},
        {lux::net::base::http_status::network_authentication_required, boost_type::network_authentication_required}};

    boost_http_response_type boost_response;
    LUX_ASSERT(status_map.contains(response.status()), "Unexpected HTTP status code in response");

    boost_response.result(status_map.at(response.status()));
    boost_response.version(response.version());

    for (const auto& [key, value] : response.headers())
    {
        boost_response.set(key, value);
    }

    boost_response.body() = lux::move(response.body());
    boost_response.prepare_payload();

    return boost_response;
}

using expiring_handler = lux::expiring_ref<lux::net::base::http_server_handler>;

class http_session : public std::enable_shared_from_this<http_session>,
                     public lux::net::base::tcp_inbound_socket_handler,
                     public http_parser_handler
{
public:
    http_session(lux::net::base::tcp_inbound_socket_ptr&& socket_ptr, const expiring_handler& handler)
        : socket_ptr_{lux::move(socket_ptr)}, handler_{handler}, parser_{*this}
    {
        LUX_ASSERT(socket_ptr_, "TCP inbound socket must not be null");
        socket_ptr_->set_handler(*this);
    }

public:
    void run()
    {
        LUX_ASSERT(socket_ptr_, "TCP inbound socket must not be null");
        self_ = shared_from_this();
        socket_ptr_->read();
    }

private:
    // lux::net::base::tcp_inbound_socket_handler implementation
    void on_disconnected(lux::net::base::tcp_inbound_socket& socket, const std::error_code& ec) override
    {
        // Do nothing, no need to handle disconnection in this case
        std::ignore = socket;
        std::ignore = ec;
        self_.reset(); // Release the shared pointer to allow session destruction
    }

    void on_data_read(lux::net::base::tcp_inbound_socket& socket, const std::span<const std::byte>& data) override
    {
        std::ignore = socket;
        parser_.parse(data);
    }

    void on_data_sent(lux::net::base::tcp_inbound_socket& socket, const std::span<const std::byte>& data) override
    {
        // Do nothing, no need to handle data sent in this case
        std::ignore = socket;
        std::ignore = data;
    }

private:
    // http_parser_handler implementation
    void on_request_parsed(const lux::net::base::http_request& request) override
    {
        if (!handler_.is_valid())
        {
            return; // Handler is no longer valid, do not process the request
        }

        auto response = handler_.get().handle_request(request);
        auto boost_response = from_lux_http_response(lux::move(response));

        using serializer_type = boost::beast::http::response_serializer<boost::beast::http::string_body>;
        auto serializer = serializer_type{lux::move(boost_response)};

        boost::system::error_code ec;
        do
        {
            serializer.next(ec, [&](boost::beast::error_code& serializer_ec, const auto& buf_seq) {
                for (const auto& buf : buf_seq)
                {
                    // Send each buffer part through the socket
                    const auto buf_view = std::span{reinterpret_cast<const std::byte*>(buf.data()), buf.size()};
                    if (const auto err = socket_ptr_->send(buf_view); err)
                    {
                        serializer_ec = err;
                    }
                    serializer.consume(buf.size());
                }
            });
        } while (!ec && !serializer.is_done());

        if (ec)
        {
            handler_.get().on_server_error(ec);
        }
    }

    void on_parse_error(const std::error_code& ec) override
    {
        if (!handler_.is_valid())
        {
            return; // Handler is no longer valid, do not process the error
        }

        handler_.get().on_server_error(ec);
    }

private:
    lux::net::base::tcp_inbound_socket_ptr socket_ptr_{nullptr};
    expiring_handler handler_;

private:
    http_parser parser_;

private:
    std::shared_ptr<http_session> self_; // To keep the session alive during async operations
};

} // namespace

class http_server::impl : public std::enable_shared_from_this<http_server::impl>,
                          public lux::net::base::tcp_acceptor_handler
{
public:
    impl(const lux::net::base::http_server_config& config,
         lux::net::base::http_server_handler& handler,
         lux::net::base::socket_factory& socket_factory)
        : handler_{handler}, acceptor_{socket_factory.create_tcp_acceptor(config.acceptor_config, *this)}
    {
    }

    impl(const lux::net::base::http_server_config& config,
         lux::net::base::http_server_handler& handler,
         lux::net::base::socket_factory& socket_factory,
         lux::net::base::ssl_context& ssl_context)
        : handler_{handler},
          acceptor_{socket_factory.create_ssl_tcp_acceptor(config.acceptor_config, ssl_context, *this)}
    {
    }

public:
    void detach_external_references()
    {
        handler_.invalidate();
    }

public:
    std::error_code serve(const lux::net::base::endpoint& ep)
    {
        return acceptor_->listen(ep);
    }

    std::error_code stop()
    {
        return acceptor_->close();
    }

    std::optional<lux::net::base::endpoint> local_endpoint() const
    {
        return acceptor_->local_endpoint();
    }

private:
    // lux::net::base::tcp_acceptor_handler implementation
    void on_accepted(lux::net::base::tcp_inbound_socket_ptr&& socket_ptr) override
    {
        if (!handler_.is_valid())
        {
            return; // Handler is no longer valid, do not process the accepted connection
        }

        std::make_shared<http_session>(lux::move(socket_ptr), handler_)->run();
    }

    void on_accept_error(const std::error_code& ec) override
    {
        if (!handler_.is_valid())
        {
            return; // Handler is no longer valid, do not process the error
        }

        handler_.get().on_server_error(ec);
    }

private:
    expiring_handler handler_;
    lux::net::base::tcp_acceptor_ptr acceptor_{nullptr};
};

http_server::http_server(const lux::net::base::http_server_config& config,
                         lux::net::base::http_server_handler& handler,
                         lux::net::base::socket_factory& socket_factory)
    : impl_{std::make_unique<impl>(config, handler, socket_factory)}
{
}

http_server::http_server(const lux::net::base::http_server_config& config,
                         lux::net::base::http_server_handler& handler,
                         lux::net::base::socket_factory& socket_factory,
                         lux::net::base::ssl_context& ssl_context)
    : impl_{std::make_unique<impl>(config, handler, socket_factory, ssl_context)}
{
}

http_server::~http_server()
{
    LUX_ASSERT(impl_, "HTTP server implementation must not be null");
    impl_->detach_external_references();
}

std::error_code lux::net::http_server::serve(const lux::net::base::endpoint& ep)
{
    LUX_ASSERT(impl_, "HTTP server implementation must not be null");
    return impl_->serve(ep);
}

std::error_code http_server::stop()
{
    LUX_ASSERT(impl_, "HTTP server implementation must not be null");
    return impl_->stop();
}

std::optional<lux::net::base::endpoint> http_server::local_endpoint() const
{
    LUX_ASSERT(impl_, "HTTP server implementation must not be null");
    return impl_->local_endpoint();
}

} // namespace lux::net
