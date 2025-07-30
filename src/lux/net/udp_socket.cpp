#include <lux/net/udp_socket.hpp>

#include <lux/logger/logger.hpp>

#include <lux/support/assert.hpp>

#include <boost/asio/ip/udp.hpp>

namespace lux::net {

class udp_socket::impl : public std::enable_shared_from_this<impl>
{
public:
    impl(boost::asio::any_io_executor exe, lux::net::base::udp_socket_handler& handler, lux::logger& logger)
        : socket_{exe}, handler_{&handler}, logger_{logger}
    {
        (void)handler_; // Suppress unused parameter warning
    }

    ~impl()
    {
    }

public:
    bool open()
    {
        try
        {
            socket_.open(boost::asio::ip::udp::v4());
            return true;
        }
        catch (const boost::system::system_error& ex)
        {
            LUX_LOG_ERROR(logger_, "Failed to open UDP socket (error={})", ex.what());
            return false;
        }
    }

    // Sending pending data before closing the socket
    bool close_gracefully()
    {
        return true;
    }

    // Forcefully closing the socket without sending pending data
    bool close_forcefully()
    {
        return true;
    }

    bool bind(const boost::asio::ip::udp::endpoint& endpoint)
    {
        try
        {
            socket_.bind(endpoint);
            return true;
        }
        catch (const boost::system::system_error& ex)
        {
            LUX_LOG_ERROR(logger_,
                          "Failed to bind UDP socket (ep={}:{}, error={})",
                          endpoint.address().to_string(),
                          endpoint.port(),
                          ex.what());
            return false;
        }
    }

    void send(const boost::asio::ip::udp::endpoint& endpoint, const std::span<const std::byte>& data)
    {
        // TODO
        (void)endpoint; // Suppress unused parameter warning
        (void)data;     // Suppress unused parameter warning
    }

private:
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint remote_endpoint_{};
    lux::net::base::udp_socket_handler* handler_{nullptr};

private:
    lux::logger& logger_;
};

udp_socket::udp_socket(boost::asio::any_io_executor exe,
                       lux::net::base::udp_socket_handler& handler,
                       lux::logger& logger)
    : impl_(std::make_shared<impl>(exe, handler, logger))
{
}

udp_socket::~udp_socket()
{
}

bool udp_socket::open()
{
    LUX_ASSERT(impl_, "UDP socket implementation must not be null");
    return impl_->open();
}

bool udp_socket::close(bool send_pending_data)
{
    LUX_ASSERT(impl_, "UDP socket implementation must not be null");

    if (send_pending_data)
    {
        return impl_->close_gracefully();
    }
    else
    {
        return impl_->close_forcefully();
    }
}

bool udp_socket::bind(const boost::asio::ip::udp::endpoint& endpoint)
{
    LUX_ASSERT(impl_, "UDP socket implementation must not be null");
    return impl_->bind(endpoint);
}

void udp_socket::send(const boost::asio::ip::udp::endpoint& endpoint, const std::span<const std::byte>& data)
{
    LUX_ASSERT(impl_, "UDP socket implementation must not be null");
    impl_->send(endpoint, data);
}

} // namespace lux::net
