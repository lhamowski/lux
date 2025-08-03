#include <lux/net/udp_socket.hpp>

#include <lux/net/base/endpoint.hpp>

#include <lux/logger/logger.hpp>
#include <lux/support/assert.hpp>
#include <lux/support/finally.hpp>
#include <lux/support/move.hpp>
#include <lux/utils/memory_arena.hpp>

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/buffer.hpp>

#include <cstring>
#include <deque>
#include <memory>
#include <span>
#include <vector>

namespace lux::net {

namespace {
constexpr std::size_t read_buffer_size = 8 * 1024; // 8 KB read buffer size

lux::net::base::endpoint from_boost_endpoint(const boost::asio::ip::udp::endpoint& ep)
{
    const auto address_uint = ep.address().to_v4().to_uint();
    const auto address_v4 = lux::net::base::address_v4{address_uint};
    return lux::net::base::endpoint{address_v4, ep.port()};
}
} // namespace

class udp_socket::impl : public std::enable_shared_from_this<impl>
{
private:
    enum class state
    {
        open,
        closing,
        closed,
    };
    state state_{state::closed};

    bool is_open() const
    {
        return state_ == state::open;
    }

    bool is_closing() const
    {
        return state_ == state::closing;
    }

    bool is_closed() const
    {
        return state_ == state::closed;
    }

public:
    impl(boost::asio::any_io_executor exe, lux::net::base::udp_socket_handler& handler, const udp_socket_config& config)
        : socket_{exe},
          handler_{&handler},
          memory_arena_{lux::make_growable_memory_arena(config.memory_arena_initial_item_count,
                                                        config.memory_arena_initial_item_size)}
    {
    }

    ~impl()
    {
        if (!is_closed())
        {
            close_immediately();
        }
    }

public:
    std::error_code open()
    {
        if (!is_closed())
        {
            return {}; // No error, already open or closing
        }

        boost::system::error_code ec;
        socket_.open(boost::asio::ip::udp::v4(), ec);
        if (ec)
        {
            return ec;
        }

        state_ = state::open;

        read(); // Start asynchronous read operation to receive data
        return {};
    }

    // Sending pending data before closing the socket
    std::error_code close_gracefully()
    {
        if (is_closed() || is_closing())
        {
            return {}; // No error, already closed or closing
        }

        if (pending_packets_.empty())
        {
            return close_immediately();
        }

        state_ = state::closing;
        return {}; // No error, closing in progress
    }

    // Immediate close without sending pending data
    std::error_code close_immediately()
    {
        if (is_closed())
        {
            return {}; // No error, already closed
        }

        state_ = state::closed;

        boost::system::error_code ec;
        socket_.close(ec);

        if (ec)
        {
            return ec;
        }

        return {};
    }

    std::error_code bind(const boost::asio::ip::udp::endpoint& endpoint)
    {
        boost::system::error_code ec;
        socket_.bind(endpoint, ec);
        return ec;
    }

    void send(const boost::asio::ip::udp::endpoint& endpoint, const std::span<const std::byte>& data)
    {
        if (is_closed())
        {
            return;
        }

        auto buffer = memory_arena_->get(data.size());
        std::memcpy(buffer->data(), data.data(), data.size());

        const bool can_send = pending_packets_.empty();
        pending_packets_.emplace_back(packet_to_send{endpoint, lux::move(buffer)});

        if (can_send)
        {
            send_next_packet();
        }
    }

    void clear_handler()
    {
        handler_ = nullptr;
    }

private:
    void read()
    {
        LUX_ASSERT(is_open(), "Cannot read from a closed UDP socket");

        socket_.async_receive_from(boost::asio::buffer(read_buffer_),
                                   sender_endpoint_,
                                   [self = shared_from_this()](const auto& ec, auto size) { self->on_read(ec, size); });
    }

    void on_read(const boost::system::error_code& ec, std::size_t size)
    {
        if (!is_open())
        {
            return;
        }

        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }

        if (handler_)
        {
            const auto ep = from_boost_endpoint(sender_endpoint_);
            if (ec)
            {
                handler_->on_read_error(from_boost_endpoint(sender_endpoint_), ec);
            }
            else
            {
                const std::span<const std::byte> data(read_buffer_.data(), size);
                handler_->on_data_read(ep, data);
            }
        }

        read(); // Continue reading for more incoming data
    }

    void send_next_packet()
    {
        if (is_closed())
        {
            return;
        }

        LUX_ASSERT(!pending_packets_.empty(), "No packets to send");

        const auto& data = *pending_packets_.front().data;
        const auto destination = pending_packets_.front().endpoint;

        socket_.async_send_to(boost::asio::buffer(data),
                              destination,
            [self = shared_from_this()](const auto& ec, [[maybe_unused]] auto size) { self->on_sent(ec); });
    }

    void on_sent(const boost::system::error_code& ec)
    {
        {
            LUX_FINALLY(pending_packets_.pop_front());

            if (ec == boost::asio::error::operation_aborted)
            {
                return;
            }

            if (is_closed())
            {
                return;
            }

            const auto& packet = pending_packets_.front();
            const auto& destination = packet.endpoint;
            const auto& data = std::span<const std::byte>(packet.data->data(), packet.data->size());

            if (handler_)
            {
                const auto ep = from_boost_endpoint(destination);

                if (!ec)
                {
                    handler_->on_data_sent(ep, data);
                }
                else
                {
                    handler_->on_send_error(ep, data, ec);
                }
            }
        }

        if (!pending_packets_.empty())
        {
            send_next_packet();
        }
        else if (is_closing())
        {
            close_immediately();
        }
    }

private:
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint sender_endpoint_{};
    lux::net::base::udp_socket_handler* handler_{nullptr};

    using arena_ptr = lux::growable_memory_arena_ptr<>;
    using arena_element = arena_ptr::element_type::element_type;
    arena_ptr memory_arena_;

    std::vector<std::byte> read_buffer_{read_buffer_size};

private:
    struct packet_to_send
    {
        boost::asio::ip::udp::endpoint endpoint;
        arena_element data; // Data to send, managed by memory arena
    };
    std::deque<packet_to_send> pending_packets_; // Queue of packets to send
};

udp_socket::udp_socket(boost::asio::any_io_executor exe,
                       lux::net::base::udp_socket_handler& handler,
                       const udp_socket_config& config)
    : impl_{std::make_shared<impl>(exe, handler, config)}
{
}

udp_socket::~udp_socket()
{
}

std::error_code udp_socket::open()
{
    LUX_ASSERT(impl_, "UDP socket implementation must not be null");
    return impl_->open();
}

std::error_code udp_socket::close(bool send_pending_data)
{
    LUX_ASSERT(impl_, "UDP socket implementation must not be null");

    if (send_pending_data)
    {
        return impl_->close_gracefully();
    }
    else
    {
        return impl_->close_immediately();
    }
}

std::error_code udp_socket::bind(const lux::net::base::endpoint& endpoint)
{
    LUX_ASSERT(impl_, "UDP socket implementation must not be null");

    const auto address = boost::asio::ip::address_v4{endpoint.address().to_uint()};
    const auto ep = boost::asio::ip::udp::endpoint{address, endpoint.port()};
    return impl_->bind(ep);
}

void udp_socket::send(const lux::net::base::endpoint& endpoint, const std::span<const std::byte>& data)
{
    LUX_ASSERT(impl_, "UDP socket implementation must not be null");

    const auto address = boost::asio::ip::address_v4{endpoint.address().to_uint()};
    const auto ep = boost::asio::ip::udp::endpoint{address, endpoint.port()};
    impl_->send(ep, data);
}

} // namespace lux::net
