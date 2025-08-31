#include <lux/io/net/tcp_socket.hpp>
#include <lux/io/net/utils.hpp>

#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/time/base/timer.hpp>
#include <lux/io/time/retry_executor.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/finally.hpp>
#include <lux/support/move.hpp>
#include <lux/support/overload.hpp>
#include <lux/utils/memory_arena.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/write.hpp>

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

class tcp_socket::impl : public std::enable_shared_from_this<impl>
{
public:
    impl(lux::net::base::tcp_socket& parent,
         boost::asio::any_io_executor exe,
         lux::net::base::tcp_socket_handler& handler,
         const lux::net::base::tcp_socket_config& config,
         lux::time::base::timer_factory& timer_factory)
        : socket_{exe},
          resolver_{exe},
          parent_{&parent},
          handler_{&handler},
          config_{config},
          memory_arena_{lux::make_growable_memory_arena(config_.buffer.initial_send_chunk_size,
                                                        config_.buffer.initial_send_chunk_count)},
          read_buffer_{config_.buffer.read_buffer_size}
    {
        if (config_.reconnect.enabled)
        {
            reconnect_executor_.emplace(timer_factory, config_.reconnect.reconnect_policy);
            reconnect_executor_->set_retry_action([this] {
                LUX_ASSERT(is_disconnected(), "Cannot reconnect when socket is not disconnected");
                reconnect();
            });
            reconnect_executor_->set_exhausted_callback([this] {
                reconnect_executor_.reset();
            });
        }
    }

    ~impl()
    {
        if (is_connected())
        {
            disconnect_immediately();
        }
    }

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

        const auto boost_ep = lux::net::to_boost_endpoint<boost::asio::ip::tcp>(endpoint);
        socket_.async_connect(boost_ep, [self = shared_from_this()](const auto& ec) { self->on_connected(ec); });
        return {};
    }

    std::error_code connect(const lux::net::base::host_endpoint& host_endpoint)
    {
        if (!is_disconnected())
        {
            return std::make_error_code(std::errc::operation_in_progress);
        }

        connect_target_ = host_endpoint;

        if (const auto ec = initialize_socket(); ec)
        {
            return ec;
        }

        state_ = state::connecting;

        // First, resolve the host and service
        resolver_.async_resolve(
            host_endpoint.host(),
            std::to_string(host_endpoint.port()),
            boost::asio::ip::resolver_base::numeric_service,
            [self = shared_from_this()](const boost::system::error_code& ec,
                                        const boost::asio::ip::tcp::resolver::results_type& results) {
                self->on_resolved(ec, results);
            });

        return {};
    }

    std::error_code disconnect(bool send_pending)
    {
        if (reconnect_executor_)
        {
            // Manual disconnection, reset the reconnect executor
            reconnect_executor_->reset();
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

        state_ = state::disconnected;

        const auto close_ec = close_socket();

        if (handler_)
        {
            LUX_ASSERT(parent_, "TCP socket parent must not be null");
            handler_->on_disconnected(*parent_, ec, will_reconnect);
        }

        return close_ec;
    }

    std::optional<lux::net::base::endpoint> local_endpoint() const
    {
        boost::system::error_code ec;
        const auto boost_local_endpoint = socket_.local_endpoint(ec);

        if (ec)
        {
            return std::nullopt; // Return empty optional if there's an error
        }

        return lux::net::from_boost_endpoint(boost_local_endpoint);
    }

    std::optional<lux::net::base::endpoint> remote_endpoint() const
    {
        boost::system::error_code ec;
        const auto boost_remote_endpoint = socket_.remote_endpoint(ec);

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

private:
    std::error_code initialize_socket()
    {
        LUX_ASSERT(!socket_.is_open(), "Socket should not be open when initializing");

        boost::system::error_code ec;
        socket_.open(boost::asio::ip::tcp::v4(), ec);
        if (ec)
        {
            return ec;
        }

        socket_.set_option(boost::asio::socket_base::keep_alive{config_.keep_alive}, ec);
        if (ec)
        {
            boost::system::error_code ignored_ec;
            socket_.close(ignored_ec);
            return ec;
        }

        return {};
    }

    std::error_code close_socket()
    {
        boost::system::error_code ec;

        if (is_connected())
        {
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            if (ec)
            {
                return ec;
            }

            socket_.cancel(ec);
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

    std::error_code shutdown_receive()
    {
        boost::system::error_code ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ec);
        return ec;
    }

    /**
     * Handles internal disconnection events due to socket errors.
     * This function is called when an error occurs during socket operations (e.g., read/write errors).
     * It performs immediate disconnection and initiates reconnection if configured.
     */
    void handle_disconnect(const std::error_code& ec)
    {
        const bool will_reconnect = reconnect_executor_.has_value();
        disconnect_immediately(ec, will_reconnect);

        if (reconnect_executor_)
        {
            reconnect_executor_->retry();
        }
    }

    void reconnect()
    {
        LUX_ASSERT(config_.reconnect.enabled, "Reconnection is not enabled in the configuration");
        LUX_ASSERT(is_disconnected(), "Cannot reconnect if not disconnected");

        auto reconnect_func = [&](const auto& endpoint) {
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

        socket_.async_read_some(boost::asio::mutable_buffer(read_buffer_.data(), read_buffer_.size()),
                                [self = shared_from_this()](const auto& ec, auto size) { self->on_read(ec, size); });
    }

    void send_next_data()
    {
        if (!is_connected() && !is_disconnecting())
        {
            return;
        }

        LUX_ASSERT(!pending_data_to_send_.empty(), "No data to send");

        auto& data = pending_data_to_send_.front();
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(*data),
                                 [self = shared_from_this()](const auto& ec, auto size) { self->on_sent(ec, size); });
    }

    void on_resolved(const boost::system::error_code& ec, const boost::asio::ip::tcp::resolver::results_type& results)
    {
        if (ec)
        {
            handle_disconnect(ec);
            return;
        }

        boost::asio::async_connect(
            socket_,
            results,
            [self = shared_from_this()](const boost::system::error_code& ec, const boost::asio::ip::tcp::endpoint& ep) {
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

        state_ = state::connected;

        if (reconnect_executor_)
        {
            // If we successfully connected, reset the reconnect executor
            reconnect_executor_->reset();
        }

        if (handler_)
        {
            LUX_ASSERT(parent_, "TCP socket parent must not be null");
            handler_->on_connected(*parent_);
        }

        read(); // Start reading data after successful connection
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
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::resolver resolver_;
    lux::net::base::tcp_socket* parent_{nullptr};
    lux::net::base::tcp_socket_handler* handler_{nullptr};
    const lux::net::base::tcp_socket_config config_;
    std::optional<lux::time::retry_executor> reconnect_executor_;

private:
    std::variant<lux::net::base::endpoint, lux::net::base::host_endpoint, std::monostate> connect_target_;
    std::optional<lux::net::base::endpoint> local_endpoint_;
    std::optional<lux::net::base::endpoint> remote_endpoint_;

private:
    using arena_ptr = lux::growable_memory_arena_ptr<>;
    using arena_element = arena_ptr::element_type::element_type;
    arena_ptr memory_arena_;
    std::deque<arena_element> pending_data_to_send_;
    std::vector<std::byte> read_buffer_;
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
}

std::error_code tcp_socket::connect(const lux::net::base::endpoint& endpoint)
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->connect(endpoint);
}

std::error_code tcp_socket::connect(const lux::net::base::host_endpoint& host_endpoint)
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");
    return impl_->connect(host_endpoint);
}

std::error_code tcp_socket::disconnect(bool send_pending)
{
    LUX_ASSERT(impl_, "TCP socket implementation must not be null");

    if (send_pending)
    {
        return impl_->disconnect_gracefully();
    }
    else
    {
        return impl_->disconnect_immediately();
    }
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

} // namespace lux::net