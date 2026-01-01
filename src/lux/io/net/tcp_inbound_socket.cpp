#include <lux/io/net/tcp_inbound_socket.hpp>
#include <lux/io/net/detail/utils.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/move.hpp>
#include <lux/utils/memory_arena.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/stream_traits.hpp>

#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <cstring>
#include <deque>
#include <memory>
#include <optional>
#include <span>
#include <system_error>
#include <vector>

namespace lux::net {

template <typename Derived>
class base_tcp_inbound_socket : public std::enable_shared_from_this<base_tcp_inbound_socket<Derived>>
{
public:
    enum class state
    {
        disconnected,
        connected,
        disconnecting,
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

public:
    void set_handler(lux::net::base::tcp_inbound_socket_handler& handler)
    {
        LUX_ASSERT(!handler_, "Handler is already set");
        handler_ = &handler;
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

    void read()
    {
        if (!is_connected())
        {
            return;
        }

        socket().async_read_some(
            boost::asio::mutable_buffer(read_buffer_.data(), read_buffer_.size()),
            [self = this->shared_from_this()](const auto& ec, auto size) { self->on_read(ec, size); });
    }

    std::error_code disconnect(bool send_pending)
    {
        if (send_pending)
        {
            return disconnect_gracefully();
        }

        return disconnect_immediately();
    }

    std::optional<lux::net::base::endpoint> local_endpoint() const
    {
        boost::system::error_code ec;
        const auto boost_local_endpoint = socket().local_endpoint(ec);

        if (ec)
        {
            return std::nullopt;
        }

        return lux::net::from_boost_endpoint(boost_local_endpoint);
    }

    std::optional<lux::net::base::endpoint> remote_endpoint() const
    {
        boost::system::error_code ec;
        const auto boost_remote_endpoint = socket().remote_endpoint(ec);

        if (ec)
        {
            return std::nullopt;
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
    base_tcp_inbound_socket(lux::net::base::tcp_inbound_socket& parent,
                            const lux::net::base::tcp_inbound_socket_config& config)
        : parent_{&parent},
          memory_arena_{lux::make_growable_memory_arena(config.buffer.initial_send_chunk_size,
                                                        config.buffer.initial_send_chunk_count)},
          read_buffer_{config.buffer.read_buffer_size}
    {
    }

private:
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
            return {};
        case state::connected:
            if (pending_data_to_send_.empty())
            {
                return disconnect_immediately();
            }

            state_ = state::disconnecting;
            return shutdown_receive();
        }

        LUX_UNREACHABLE();
    }

    std::error_code disconnect_immediately(const std::error_code& ec = {})
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
            handler_->on_disconnected(*parent_, ec);
        }

        return close_ec;
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

    void on_read(const boost::system::error_code& ec, std::size_t size)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }

        if (!is_connected() && !is_disconnecting())
        {
            return;
        }

        if (ec)
        {
            disconnect_immediately(ec);
            return;
        }

        if (size > 0 && handler_)
        {
            std::span<const std::byte> data{read_buffer_.data(), size};

            LUX_ASSERT(parent_, "TCP inbound socket parent must not be null");
            handler_->on_data_read(*parent_, data);
        }

        read();
    }

    void on_sent(const boost::system::error_code& ec, std::size_t size)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }

        if (ec)
        {
            disconnect_immediately(ec);
            return;
        }

        if (!is_connected() && !is_disconnecting())
        {
            return;
        }

        if (handler_)
        {
            LUX_ASSERT(parent_, "TCP inbound socket parent must not be null");
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
            disconnect_immediately();
        }
    }

private:
    lux::net::base::tcp_inbound_socket* parent_{nullptr};
    lux::net::base::tcp_inbound_socket_handler* handler_{nullptr};

private:
    using arena_ptr = lux::growable_memory_arena_ptr<>;
    using arena_element = arena_ptr::element_type::element_type;
    arena_ptr memory_arena_;
    std::deque<arena_element> pending_data_to_send_;
    std::vector<std::byte> read_buffer_;
};

class tcp_inbound_socket::impl : public base_tcp_inbound_socket<tcp_inbound_socket::impl>
{
public:
    impl(boost::asio::ip::tcp::socket&& socket,
         lux::net::tcp_inbound_socket& parent,
         const lux::net::base::tcp_inbound_socket_config& config)
        : base_tcp_inbound_socket<tcp_inbound_socket::impl>{parent, config}, socket_{lux::move(socket)}
    {
        state_ = state::connected;
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
        return ec;
    }

private:
    boost::asio::ip::tcp::socket socket_;
};

tcp_inbound_socket::tcp_inbound_socket(boost::asio::ip::tcp::socket&& socket,
                                       const lux::net::base::tcp_inbound_socket_config& config)
    : impl_{std::make_shared<impl>(lux::move(socket), *this, config)}
{
}

tcp_inbound_socket::~tcp_inbound_socket()
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    impl_->detach_external_references();
    disconnect(false);
}

void tcp_inbound_socket::set_handler(lux::net::base::tcp_inbound_socket_handler& handler)
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    impl_->set_handler(handler);
}

std::error_code tcp_inbound_socket::send(const std::span<const std::byte>& data)
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    return impl_->send(data);
}

void tcp_inbound_socket::read()
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    impl_->read();
}

std::error_code tcp_inbound_socket::disconnect(bool send_pending)
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    return impl_->disconnect(send_pending);
}

bool tcp_inbound_socket::is_connected() const
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    return impl_->is_connected();
}

std::optional<lux::net::base::endpoint> tcp_inbound_socket::local_endpoint() const
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    return impl_->local_endpoint();
}

std::optional<lux::net::base::endpoint> tcp_inbound_socket::remote_endpoint() const
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    return impl_->remote_endpoint();
}

class ssl_tcp_inbound_socket::impl : public base_tcp_inbound_socket<ssl_tcp_inbound_socket::impl>
{
public:
    impl(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>&& stream,
         lux::net::base::tcp_inbound_socket& parent,
         const lux::net::base::tcp_inbound_socket_config& config)
        : base_tcp_inbound_socket<ssl_tcp_inbound_socket::impl>(parent, config), stream_{lux::move(stream)}
    {
        state_ = state::connected;
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
        stream_.async_shutdown([self = shared_from_base()](const auto& ec) { self->on_shutdowned(ec); });
        return {};
    }

private:
    void on_shutdowned(const boost::system::error_code& ec)
    {
        if (ec)
        {
            if (ec != boost::asio::ssl::error::stream_truncated)
            {
                // TODO: inform handler if needed
            }
        }
    }

private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream_;
};

ssl_tcp_inbound_socket::ssl_tcp_inbound_socket(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>&& stream,
                                               const lux::net::base::tcp_inbound_socket_config& config)
    : impl_{std::make_shared<impl>(lux::move(stream), *this, config)}
{
}

void ssl_tcp_inbound_socket::set_handler(lux::net::base::tcp_inbound_socket_handler& handler)
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    impl_->set_handler(handler);
}

ssl_tcp_inbound_socket::~ssl_tcp_inbound_socket()
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    impl_->detach_external_references();
    disconnect(false);
}

std::error_code ssl_tcp_inbound_socket::send(const std::span<const std::byte>& data)
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    return impl_->send(data);
}

void ssl_tcp_inbound_socket::read()
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    impl_->read();
}

std::error_code ssl_tcp_inbound_socket::disconnect(bool send_pending)
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    return impl_->disconnect(send_pending);
}

bool ssl_tcp_inbound_socket::is_connected() const
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    return impl_->is_connected();
}

std::optional<lux::net::base::endpoint> ssl_tcp_inbound_socket::local_endpoint() const
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    return impl_->local_endpoint();
}

std::optional<lux::net::base::endpoint> ssl_tcp_inbound_socket::remote_endpoint() const
{
    LUX_ASSERT(impl_, "TCP inbound socket implementation must not be null");
    return impl_->remote_endpoint();
}

} // namespace lux::net
