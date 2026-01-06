#include <lux/io/net/http_server.hpp>

#include <lux/io/net/base/http_request.hpp>
#include <lux/io/net/base/http_response.hpp>
#include <lux/io/net/base/http_status.hpp>
#include <lux/io/net/base/socket_factory.hpp>
#include <lux/io/net/base/tcp_acceptor.hpp>
#include <lux/io/net/base/tcp_socket.hpp>

#include <lux/io/net/detail/http_parser.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/move.hpp>
#include <lux/support/expiring_ref.hpp>
#include <lux/support/finally.hpp>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/string_body.hpp>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace lux::net {

namespace {

using boost_http_response_type = detail::boost_http_response_type;

lux::net::base::http_request from_boost_http_request(detail::boost_http_request_type&& boost_request)
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

class server_request_parser_handler : public detail::http_request_parser_handler
{
public:
    virtual void on_request_parsed(const lux::net::base::http_request& request) = 0;

public:
    void on_message_parsed(detail::boost_http_request_type&& message) override final
    {
        on_request_parsed(from_boost_http_request(lux::move(message)));
    }
};

boost_http_response_type from_lux_http_response(lux::net::base::http_response&& response)
{
    using boost_status_type = boost::beast::http::status;
    using lux_status_type = lux::net::base::http_status;

    static const std::unordered_map<lux::net::base::http_status, boost_status_type> status_map = {
        {lux_status_type::unknown, boost_status_type::unknown},

        {lux_status_type::continue_, boost_status_type::continue_},
        {lux_status_type::switching_protocols, boost_status_type::switching_protocols},
        {lux_status_type::processing, boost_status_type::processing},
        {lux_status_type::early_hints, boost_status_type::early_hints},

        {lux_status_type::ok, boost_status_type::ok},
        {lux_status_type::created, boost_status_type::created},
        {lux_status_type::accepted, boost_status_type::accepted},
        {lux_status_type::non_authoritative_information, boost_status_type::non_authoritative_information},
        {lux_status_type::no_content, boost_status_type::no_content},
        {lux_status_type::reset_content, boost_status_type::reset_content},
        {lux_status_type::partial_content, boost_status_type::partial_content},
        {lux_status_type::multi_status, boost_status_type::multi_status},
        {lux_status_type::already_reported, boost_status_type::already_reported},
        {lux_status_type::im_used, boost_status_type::im_used},

        {lux_status_type::multiple_choices, boost_status_type::multiple_choices},
        {lux_status_type::moved_permanently, boost_status_type::moved_permanently},
        {lux_status_type::found, boost_status_type::found},
        {lux_status_type::see_other, boost_status_type::see_other},
        {lux_status_type::not_modified, boost_status_type::not_modified},
        {lux_status_type::use_proxy, boost_status_type::use_proxy},
        {lux_status_type::temporary_redirect, boost_status_type::temporary_redirect},
        {lux_status_type::permanent_redirect, boost_status_type::permanent_redirect},

        {lux_status_type::bad_request, boost_status_type::bad_request},
        {lux_status_type::unauthorized, boost_status_type::unauthorized},
        {lux_status_type::payment_required, boost_status_type::payment_required},
        {lux_status_type::forbidden, boost_status_type::forbidden},
        {lux_status_type::not_found, boost_status_type::not_found},
        {lux_status_type::method_not_allowed, boost_status_type::method_not_allowed},
        {lux_status_type::not_acceptable, boost_status_type::not_acceptable},
        {lux_status_type::proxy_authentication_required, boost_status_type::proxy_authentication_required},
        {lux_status_type::request_timeout, boost_status_type::request_timeout},
        {lux_status_type::conflict, boost_status_type::conflict},
        {lux_status_type::gone, boost_status_type::gone},
        {lux_status_type::length_required, boost_status_type::length_required},
        {lux_status_type::precondition_failed, boost_status_type::precondition_failed},
        {lux_status_type::payload_too_large, boost_status_type::payload_too_large},
        {lux_status_type::uri_too_long, boost_status_type::uri_too_long},
        {lux_status_type::unsupported_media_type, boost_status_type::unsupported_media_type},
        {lux_status_type::range_not_satisfiable, boost_status_type::range_not_satisfiable},
        {lux_status_type::expectation_failed, boost_status_type::expectation_failed},
        {lux_status_type::misdirected_request, boost_status_type::misdirected_request},
        {lux_status_type::unprocessable_entity, boost_status_type::unprocessable_entity},
        {lux_status_type::locked, boost_status_type::locked},
        {lux_status_type::failed_dependency, boost_status_type::failed_dependency},
        {lux_status_type::too_early, boost_status_type::too_early},
        {lux_status_type::upgrade_required, boost_status_type::upgrade_required},
        {lux_status_type::precondition_required, boost_status_type::precondition_required},
        {lux_status_type::too_many_requests, boost_status_type::too_many_requests},
        {lux_status_type::request_header_fields_too_large, boost_status_type::request_header_fields_too_large},
        {lux_status_type::unavailable_for_legal_reasons, boost_status_type::unavailable_for_legal_reasons},

        {lux_status_type::internal_server_error, boost_status_type::internal_server_error},
        {lux_status_type::not_implemented, boost_status_type::not_implemented},
        {lux_status_type::bad_gateway, boost_status_type::bad_gateway},
        {lux_status_type::service_unavailable, boost_status_type::service_unavailable},
        {lux_status_type::gateway_timeout, boost_status_type::gateway_timeout},
        {lux_status_type::http_version_not_supported, boost_status_type::http_version_not_supported},
        {lux_status_type::variant_also_negotiates, boost_status_type::variant_also_negotiates},
        {lux_status_type::insufficient_storage, boost_status_type::insufficient_storage},
        {lux_status_type::loop_detected, boost_status_type::loop_detected},
        {lux_status_type::not_extended, boost_status_type::not_extended},
        {lux_status_type::network_authentication_required, boost_status_type::network_authentication_required}};

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

class http_session;
using session_unregister_callback = std::function<void(http_session*)>;

class http_session : public std::enable_shared_from_this<http_session>,
                     public lux::net::base::tcp_inbound_socket_handler,
                     public server_request_parser_handler
{
public:
    http_session(lux::net::base::tcp_inbound_socket_ptr&& socket_ptr,
                 const expiring_handler& handler,
                 session_unregister_callback unregister_callback)
        : socket_ptr_{lux::move(socket_ptr)},
          handler_{handler},
          parser_{*this},
          unregister_callback_{lux::move(unregister_callback)}
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

    void close()
    {
        if (state_ == state::closed || state_ == state::closing)
        {
            return;
        }

        if (state_ == state::responding)
        {
            set_state(state::closing);
            return;
        }

        // If we are not currently responding, we can disconnect immediately
        set_state(state::closing);
        if (socket_ptr_)
        {
            socket_ptr_->disconnect(true);
        }
    }

private:
    // lux::net::base::tcp_inbound_socket_handler implementation
    void on_disconnected(lux::net::base::tcp_inbound_socket& socket, const std::error_code& ec) override
    {
        set_state(state::closed);

        std::ignore = socket;
        std::ignore = ec;

        if (unregister_callback_)
        {
            unregister_callback_(this);
        }

        self_.reset(); // Release the shared pointer to allow session destruction
    }

    void on_data_read(lux::net::base::tcp_inbound_socket& socket, const std::span<const std::byte>& data) override
    {
        std::ignore = socket;

        set_state(state::parsing);
        parser_.parse(data);
    }

    void on_data_sent(lux::net::base::tcp_inbound_socket& socket, const std::span<const std::byte>& data) override
    {
        std::ignore = socket;
        std::ignore = data;

        set_state(state::idle);
    }

private:
    // http_request_parser_handler implementation
    void on_request_parsed(const lux::net::base::http_request& request) override
    {
        if (!handler_.is_valid())
        {
            return; // Handler is no longer valid, do not process the request
        }

        set_state(state::responding);

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
            if (handler_.is_valid())
            {
                handler_.get().on_server_error(ec);
            }
        }

        if (state_ == state::closing)
        {
            socket_ptr_->disconnect(true);
        }
    }

    void on_parse_error(const std::error_code& ec) override
    {
        if (!handler_.is_valid())
        {
            return; // Handler is no longer valid, do not process the error
        }

        set_state(state::idle);
        handler_.get().on_server_error(ec);
    }

private:
    enum class state
    {
        idle,
        parsing,
        responding,
        closing,
        closed
    };
    state state_{state::idle};

    void set_state(state new_state)
    {
        state_ = new_state;
    }

private:
    lux::net::base::tcp_inbound_socket_ptr socket_ptr_{nullptr};
    expiring_handler handler_;

private:
    detail::http_request_parser parser_;

private:
    session_unregister_callback unregister_callback_;
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
    std::error_code serve(const lux::net::base::endpoint& ep)
    {
        return acceptor_->listen(ep);
    }

    std::error_code stop()
    {
        close_all_sessions();
        return acceptor_->close();
    }

    std::optional<lux::net::base::endpoint> local_endpoint() const
    {
        return acceptor_->local_endpoint();
    }

    void detach_external_references()
    {
        handler_.invalidate();
    }

private:
    void close_all_sessions()
    {
        std::vector<std::shared_ptr<http_session>> sessions_to_close;

        {
            std::lock_guard lock{sessions_mutex_};
            sessions_to_close.reserve(sessions_.size());
            for (auto& [_, session] : sessions_)
            {
                if (auto s = session.lock())
                {
                    sessions_to_close.push_back(lux::move(s));
                }
            }
            sessions_.clear();
        }

        // Close sessions outside the lock to avoid deadlock
        for (auto& session : sessions_to_close)
        {
            session->close();
        }
    }

private:
    void unregister_session(http_session* session_addr)
    {
        std::lock_guard lock{sessions_mutex_};
        sessions_.erase(session_addr);
    }

    // lux::net::base::tcp_acceptor_handler implementation
    void on_accepted(lux::net::base::tcp_inbound_socket_ptr&& socket_ptr) override
    {
        if (!handler_.is_valid())
        {
            return;
        }

        auto session = std::make_shared<http_session>(lux::move(socket_ptr), handler_, [this](http_session* addr) {
            unregister_session(addr);
        });

        {
            std::lock_guard lock{sessions_mutex_};
            sessions_.emplace(session.get(), session);
        }

        session->run();
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

    std::recursive_mutex sessions_mutex_;
    std::unordered_map<http_session*, std::weak_ptr<http_session>> sessions_;
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
    impl_->stop();
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
