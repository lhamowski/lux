#include <lux/io/net/tcp_socket.hpp>
#include <lux/io/net/utils.hpp>

#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/time/base/timer.hpp>
#include <lux/io/time/retry_executor.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/move.hpp>
#include <lux/support/overload.hpp>
#include <lux/utils/memory_arena.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/write.hpp>

#include <boost/beast/core/stream_traits.hpp>

#include <cstring>
#include <deque>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <variant>
#include <vector>

namespace lux::net {

template <typename Derived>
class base_tcp_socket : public std::enable_shared_from_this<base_tcp_socket<Derived>>
{
public:
    enum class state
    {
        disconnected,
        connected,
        disconnecting,
        connecting,
    };

    state state_{state::disconnected};

    bool is_disconnected() const
    {
        return state_ == state::disconnected;
    }

    bool is_connected() const
    {
        return state_ == state::connected;
    }

    bool is_disconnecting() const
    {
        return state_ == state::disconnecting;
    }

    bool is_connecting() const
    {
        return state_ == state::connecting;
    }

public:
    std::error_code connect(const lux::net::base::endpoint& endpoint)
    {
        if (!is_disconnected())
        {
            return std::make_error_code(std::errc::operation_in_progress);
        }

        connect_target_ = endpoint;

        if (const auto ec = initialize_socket(); ec)
        {
            return ec;
        }

        state_ = state::connecting;

        if (reconnect_executor_ && reconnect_executor_->is_retry_exhausted())
        {
            // This means that previous reconnect attempts have failed, but now we are doing a manual connect, so reset
            // the reconnect executor
            reconnect_executor_->reset();
        }

        const auto boost_ep = lux::net::to_boost_endpoint<boost::asio::ip::tcp>(endpoint);
        socket().async_connect(boost_ep, [self = this->shared_from_this()](const auto& ec) { self->on_connected(ec); });
        return {};
    }

    std::error_code connect(const lux::net::base::hostname_endpoint& hostname_endpoint)
    {
        if (!is_disconnected())
        {
            return std::make_error_code(std::errc::operation_in_progress);
        }

        connect_target_ = hostname_endpoint;

        if (const auto ec = initialize_socket(); ec)
        {
            return ec;
        }

        state_ = state::connecting;

        if (reconnect_executor_ && reconnect_executor_->is_retry_exhausted())
        {
            // This means that previous reconnect attempts have failed, but now we are doing a manual connect, so reset
            // the reconnect executor
            reconnect_executor_->reset();
        }

        // First, resolve the host and service
        resolver_.async_resolve(
            hostname_endpoint.host(),
            std::to_string(hostname_endpoint.port()),
            boost::asio::ip::resolver_base::numeric_service,
            [self = this->shared_from_this()](const boost::system::error_code& ec,
                                              const boost::asio::ip::tcp::resolver::results_type& results) {
                self->on_resolved(ec, results);
            });

        return {};
    }

    std::error_code disconnect(bool send_pending)
    {
        if (reconnect_executor_)
        {
            // Manual disconnection, cancel any pending reconnect attempts
            reconnect_executor_->cancel();
        }

        if (send_pending)
        {
            return disconnect_gracefully();
        }
        else
        {
            return disconnect_immediately();
        }
    }

    std::error_code send(const std::span<const std::byte>& data)
    {
        if (!is_connected() && !is_disconnecting())
        {
            return std::make_error_code(std::errc::not_connected);
        }

        if (data.empty())
        {
            return std::make_error_code(std::errc::invalid_argument);
        }

        auto buffer = memory_arena_->get(data.size());
        std::memcpy(buffer->data(), data.data(), data.size());

        const bool can_send = pending_data_to_send_.empty();
        pending_data_to_send_.emplace_back(lux::move(buffer));

        if (can_send)
        {
            send_next_data();
        }

        return {};
    }

    std::optional<lux::net::base::endpoint> local_endpoint() const
    {
        boost::system::error_code ec;
        const auto boost_local_endpoint = socket().local_endpoint(ec);

        if (ec)
        {
            return std::nullopt; // Return empty optional if there's an error
        }

        return lux::net::from_boost_endpoint(boost_local_endpoint);
    }

    std::optional<lux::net::base::endpoint> remote_endpoint() const
    {
        boost::system::error_code ec;
        const auto boost_remote_endpoint = socket().remote_endpoint(ec);

        if (ec)
        {
            return std::nullopt; // Return empty optional if there's an error
        }

        return lux::net::from_boost_endpoint(boost_remote_endpoint);
    }

    /**
     * Clears the handler and parent references.
     * This is used to prevent dangling pointers when the parent is destroyed but the impl is still alive (e.g., in
     * completion handlers).
     */
    void detach_external_references()
    {
        handler_ = nullptr;
        parent_ = nullptr;
    }

protected:
    std::shared_ptr<Derived> shared_from_base()
    {
        // Safe to use static_pointer_cast because of CRTP pattern
        return std::static_pointer_cast<Derived>(this->shared_from_this());
    }

private:
    base_tcp_socket(lux::net::base::tcp_socket& parent,
                    boost::asio::any_io_executor exe,
                    lux::net::base::tcp_socket_handler& handler,
                    const lux::net::base::tcp_socket_config& config,
                    lux::time::base::timer_factory& timer_factory)
        : resolver_{exe},
          parent_{&parent},
          handler_{&handler},
          config_{config},
          memory_arena_{lux::make_growable_memory_arena(config_.buffer.initial_send_chunk_size,
                                                        config_.buffer.initial_send_chunk_count)},
          read_buffer_{config_.buffer.read_buffer_size},
          timer_factory_{timer_factory}
    {
        if (config_.reconnect.enabled)
        {
            reconnect_executor_.emplace(timer_factory_, config_.reconnect.reconnect_policy);
            reconnect_executor_->set_retry_action([this] {
                LUX_ASSERT(is_disconnected(), "Cannot reconnect when socket is not disconnected");
                reconnect();
            });
        }
    }

private:
    // Allow Derived to create instances of this base class (CRTP pattern)
    friend Derived;

    Derived& derived()
    {
        return static_cast<Derived&>(*this);
    }

    const Derived& derived() const
    {
        return static_cast<const Derived&>(*this);
    }

    auto& socket()
    {
        return boost::beast::get_lowest_layer(derived().stream());
    }

    const auto& socket() const
    {
        return boost::beast::get_lowest_layer(derived().stream());
    }

private:
    std::error_code initialize_socket()
    {
        LUX_ASSERT(!socket().is_open(), "Socket should not be open when initializing");

        boost::system::error_code ec;
        socket().open(boost::asio::ip::tcp::v4(), ec);
        if (ec)
        {
            return ec;
        }

        socket().set_option(boost::asio::socket_base::keep_alive{config_.keep_alive}, ec);
        if (ec)
        {
            boost::system::error_code ignored_ec;
            socket().close(ignored_ec);
            return ec;
        }

        return {};
    }

    std::error_code close_socket()
    {
        return derived().close();
    }

    std::error_code shutdown_receive()
    {
        boost::system::error_code ec;
        socket().shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ec);
        return ec;
    }

    std::error_code disconnect_gracefully()
    {
        switch (state_)
        {
        case state::disconnected:
        case state::disconnecting:
            return {}; // No error, already disconnected or disconnecting
        case state::connected:
            if (pending_data_to_send_.empty())
            {
                return disconnect_immediately(); // No pending data, disconnect immediately
            }

            state_ = state::disconnecting;
            return shutdown_receive(); // Shutdown receive to stop reading data
        case state::connecting:
            return disconnect_immediately();
        }

        LUX_UNREACHABLE();
    }

    std::error_code disconnect_immediately(const std::error_code& ec = {}, bool will_reconnect = false)
    {
        if (is_disconnected())
        {
            return {}; // No error, already disconnected
        }

        const auto close_ec = close_socket();
        state_ = state::disconnected;

        if (handler_)
        {
            LUX_ASSERT(parent_, "TCP socket parent must not be null");
            handler_->on_disconnected(*parent_, ec, will_reconnect);
        }

        return close_ec;
    }

    /**
     * Handles internal disconnection events due to socket errors.
     * This function is called when an error occurs during socket operations (e.g., read/write errors).
     * It performs immediate disconnection and initiates reconnection if configured.
     */
    void handle_disconnect(const std::error_code& ec)
    {
        const bool will_reconnect = reconnect_executor_.has_value() && !reconnect_executor_->is_retry_exhausted();
        disconnect_immediately(ec, will_reconnect);

        // we need to check again if we will reconnect, as the disconnect_immediately call may have cancelled the
        // reconnect executor
        const bool will_reconnect_after_callback = reconnect_executor_.has_value() &&
                                                   !reconnect_executor_->is_retry_exhausted();
        if (will_reconnect_after_callback)
        {
            reconnect_executor_->retry();
        }
    }

    void reconnect()
    {
        LUX_ASSERT(config_.reconnect.enabled, "Reconnection is not enabled in the configuration");
        LUX_ASSERT(is_disconnected(), "Cannot reconnect if not disconnected");

        auto reconnect_func = [&](const auto& endpoint) {
            boost::system::error_code ignored_ec;
            socket().close(ignored_ec); // Ensure socket is closed before reconnecting

            if (const auto ec = connect(endpoint); ec)
            {
                // If connection fails immediately, retry using the reconnect executor
                disconnect_immediately(ec, true);
                reconnect_executor_->retry();
            }
        };

        auto overload = lux::overload{lux::move(reconnect_func),
                                      [](const std::monostate&) { LUX_ASSERT(false, "invalid target"); }};

        std::visit(lux::move(overload), connect_target_);
    }

    void read()
    {
        if (!is_connected())
        {
            return; // Cannot read if not connected
        }

        socket().async_read_some(
            boost::asio::mutable_buffer(read_buffer_.data(), read_buffer_.size()),
            [self = this->shared_from_this()](const auto& ec, auto size) { self->on_read(ec, size); });
    }

    void send_next_data()
    {
        if (!is_connected() && !is_disconnecting())
        {
            return;
        }

        LUX_ASSERT(!pending_data_to_send_.empty(), "No data to send");

        auto& data = pending_data_to_send_.front();
        boost::asio::async_write(
            socket(),
            boost::asio::buffer(*data),
            [self = this->shared_from_this()](const auto& ec, auto size) { self->on_sent(ec, size); });
    }

    void on_resolved(const boost::system::error_code& ec, const boost::asio::ip::tcp::resolver::results_type& results)
    {
        if (ec)
        {
            handle_disconnect(ec);
            return;
        }

        boost::asio::async_connect(socket(),
                                   results,
                                   [self = this->shared_from_this()](const boost::system::error_code& ec,
                                                                     const boost::asio::ip::tcp::endpoint& ep) {
                                       (void)ep; // Ignore the endpoint, we only care about the error code
                                       self->on_connected(ec);
                                   });
    }

    void on_connected(const boost::system::error_code& ec)
    {
        if (!is_connecting())
        {
            return; // Ignore if not in connecting state
        }

        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }

        if (ec)
        {
            handle_disconnect(ec);
            return;
        }

        if (reconnect_executor_)
        {
            // If we successfully connected, reset the reconnect executor
            reconnect_executor_->reset();
        }

        derived().on_connection_established();
    }

    void on_read(const boost::system::error_code& ec, std::size_t size)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            return; // Operation was cancelled
        }

        // If in the meantime the socket was disconnected, ignore the read
        if (!is_connected() && !is_disconnecting())
        {
            return;
        }

        if (ec)
        {
            handle_disconnect(ec);
            return;
        }

        if (size > 0 && handler_)
        {
            std::span<const std::byte> data{read_buffer_.data(), size};

            LUX_ASSERT(parent_, "TCP socket parent must not be null");
            handler_->on_data_read(*parent_, data);
        }

        read();
    }

    void on_sent(const boost::system::error_code& ec, std::size_t size)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            return; // Operation was cancelled
        }

        if (ec)
        {
            handle_disconnect(ec);
            return;
        }

        // If in the meantime the socket was disconnected, ignore the send
        if (!is_connected() && !is_disconnecting())
        {
            return;
        }

        if (handler_)
        {
            LUX_ASSERT(parent_, "TCP socket parent must not be null");
            const auto& last_data_sent = *pending_data_to_send_.front();
            const auto& last_data_view = std::span<const std::byte>(last_data_sent.data(), size);
            handler_->on_data_sent(*parent_, last_data_view);
        }

        pending_data_to_send_.pop_front();
        if (!pending_data_to_send_.empty())
        {
            send_next_data();
        }
        else if (is_disconnecting())
        {
            // If we are disconnecting and there is no more pending data, close the socket immediately
            disconnect_immediately();
        }
    }

private:
    boost::asio::ip::tcp::resolver resolver_;
    lux::net::base::tcp_socket* parent_{nullptr};
    lux::net::base::tcp_socket_handler* handler_{nullptr};
    const lux::net::base::tcp_socket_config config_;
    std::optional<lux::time::retry_executor> reconnect_executor_;

private:
    std::variant<lux::net::base::endpoint, lux::net::base::hostname_endpoint, std::monostate> connect_target_;
    std::optional<lux::net::base::endpoint> local_endpoint_;
    std::optional<lux::net::base::endpoint> remote_endpoint_;

private:
    using arena_ptr = lux::growable_memory_arena_ptr<>;
    using arena_element = arena_ptr::element_type::element_type;
    arena_ptr memory_arena_;
    std::deque<arena_element> pending_data_to_send_;
    std::vector<std::byte> read_buffer_;

private:
    lux::time::base::timer_factory& timer_factory_;
};

class tcp_socket::impl : public base_tcp_socket<tcp_socket::impl>
{
public:
    impl(lux::net::tcp_socket& parent,
         boost::asio::any_io_executor exe,
         lux::net::base::tcp_socket_handler& handler,
         const lux::net::base::tcp_socket_config& config,
         lux::time::base::timer_factory& timer_factory)
        : base_tcp_socket<tcp_socket::impl>(parent, exe, handler, config, timer_factory), socket_{exe}
    {
    }

public:
    auto& stream()
    {
        return socket_;
    }

    const auto& stream() const
    {
        return socket_;
    }

public:
    std::error_code close()
    {
        boost::system::error_code ec;

        if (is_connected())
        {
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            if (ec)
            {
                return ec;
            }
        }

        socket_.close(ec);
        if (ec)
        {
            return ec;
        }

        return {};
    }

public:
    void on_connection_established()
    {
        state_ = state::connected;

        if (handler_)
        {
            LUX_ASSERT(parent_, "TCP socket parent must not be null");
            handler_->on_connected(*parent_);
        }

        read();
    }

private:
    boost::asio::ip::tcp::socket socket_;
};

tcp_socket::tcp_socket(boost::asio::any_io_executor exe,
                       lux::net::base::tcp_socket_handler& handler,
                       const lux::net::base::tcp_socket_config& config,
                       lux::time::base::timer_factory& timer_factory)
    : impl_{std::make_shared<impl>(*this, exe, handler, config, timer_factory)}
{
}

tcp_socket::~tcp_socket()
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    impl_->detach_external_references();
    disconnect(false);
}

std::error_code tcp_socket::connect(const lux::net::base::endpoint& endpoint)
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->connect(endpoint);
}

std::error_code tcp_socket::connect(const lux::net::base::hostname_endpoint& hostname_endpoint)
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->connect(hostname_endpoint);
}

std::error_code tcp_socket::disconnect(bool send_pending)
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->disconnect(send_pending);
}

std::error_code tcp_socket::send(const std::span<const std::byte>& data)
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->send(data);
}

bool tcp_socket::is_connected() const
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->is_connected();
}

std::optional<lux::net::base::endpoint> tcp_socket::local_endpoint() const
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->local_endpoint();
}

std::optional<lux::net::base::endpoint> tcp_socket::remote_endpoint() const
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->remote_endpoint();
}

class ssl_tcp_socket::impl : public base_tcp_socket<ssl_tcp_socket::impl>
{
public:
    impl(lux::net::base::tcp_socket& parent,
         boost::asio::any_io_executor exe,
         lux::net::base::tcp_socket_handler& handler,
         const lux::net::base::tcp_socket_config& config,
         lux::time::base::timer_factory& timer_factory,
         lux::net::base::ssl_context& ssl_context,
         lux::net::base::ssl_mode ssl_mode)
        : base_tcp_socket<ssl_tcp_socket::impl>(parent, exe, handler, config, timer_factory),
          stream_{exe, ssl_context},
          ssl_mode_{ssl_mode}
    {
    }

public:
    auto& stream()
    {
        return stream_;
    }

    const auto& stream() const
    {
        return stream_;
    }

public:
    std::error_code close()
    {
        // Perform the SSL shutdown
        stream_.async_shutdown([self = shared_from_base()](const auto& ec) { self->on_shutdowned(ec); });
        return {};
    }

public:
    void on_connection_established()
    {
        handshake();
    }

private:
    void handshake()
    {
        const auto handshake_type = (ssl_mode_ == lux::net::base::ssl_mode::client)
                                        ? boost::asio::ssl::stream_base::client
                                        : boost::asio::ssl::stream_base::server;
        stream_.async_handshake(handshake_type,
                                [self = shared_from_base()](const auto& ec) { self->on_handshake_completed(ec); });
    }

private:
    void on_handshake_completed(const boost::system::error_code& ec)
    {
        if (ec)
        {
            handle_disconnect(ec);
            return;
        }

        // We want to mark the socket as connected only after the handshake is complete, even though the underlying TCP
        // connection is already established. This ensures that any data sent or received is properly
        // encrypted/decrypted.
        state_ = state::connected;

        if (handler_)
        {
            LUX_ASSERT(parent_, "TCP socket parent must not be null");
            handler_->on_connected(*parent_);
        }

        read();
    }

    void on_shutdowned(const boost::system::error_code& ec)
    {
        if (ec)
        {
            // Non-compliant servers don't participate in the SSL/TLS shutdown process and
            // close the underlying transport layer. This causes the shutdown operation to
            // complete with a `stream_truncated` error. One might decide not to log such
            // errors as there are many non-compliant servers in the wild.
            if (ec != boost::asio::ssl::error::stream_errors::stream_truncated)
            {
                // TODO: inform the handler about the shutdown error
            }
        }
    }

private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream_;
    lux::net::base::ssl_mode ssl_mode_;
};

ssl_tcp_socket::ssl_tcp_socket(boost::asio::any_io_executor exe,
                               lux::net::base::tcp_socket_handler& handler,
                               const lux::net::base::tcp_socket_config& config,
                               lux::time::base::timer_factory& timer_factory,
                               lux::net::base::ssl_context& ssl_context,
                               lux::net::base::ssl_mode ssl_mode)
    : impl_{std::make_shared<impl>(*this, exe, handler, config, timer_factory, ssl_context, ssl_mode)}
{
}

ssl_tcp_socket::~ssl_tcp_socket()
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    impl_->detach_external_references();
    disconnect(false);
}

std::error_code ssl_tcp_socket::connect(const lux::net::base::endpoint& endpoint)
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->connect(endpoint);
}

std::error_code ssl_tcp_socket::connect(const lux::net::base::hostname_endpoint& hostname_endpoint)
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->connect(hostname_endpoint);
}

std::error_code ssl_tcp_socket::disconnect(bool send_pending)
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->disconnect(send_pending);
}

std::error_code ssl_tcp_socket::send(const std::span<const std::byte>& data)
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->send(data);
}

bool ssl_tcp_socket::is_connected() const
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->is_connected();
}

std::optional<lux::net::base::endpoint> ssl_tcp_socket::local_endpoint() const
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->local_endpoint();
}

std::optional<lux::net::base::endpoint> ssl_tcp_socket::remote_endpoint() const
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->remote_endpoint();
}

} // namespace lux::net