#pragma once

#include <boost/asio/ip/udp.hpp>

#include <span>

namespace lux::net::base {

class udp_socket_handler
{
public:
    /**
     * Called when data is received on the UDP socket.
     * @param endpoint The endpoint from which the data was received.
     * @param data The received data, represented as a span of bytes.
     */
    virtual void on_data_received(const boost::asio::ip::udp::endpoint& endpoint,
                                  const std::span<const std::byte>& data) = 0;

protected:
    virtual ~udp_socket_handler() = default;
};

class udp_socket
{
public:
    virtual ~udp_socket() = default;

public:
    /**
     * Opens the UDP socket.
     * @return true if the socket was successfully opened, false otherwise.
     */
    virtual bool open() = 0;

    /**
     * Closes the UDP socket.
     *
     * @param send_pending_data If true, any pending data will be sent before closing the socket.
     * @return true if the socket was successfully closed, false otherwise.
     */
    virtual bool close(bool send_pending_data) = 0;

    /**
     * Asynchronously reads data from the UDP socket.
     * This method should be called to start reading data from the socket.
     * It will invoke the handler's on_data_received method when data is received.
     */
    virtual void read() = 0;

    /**
     * Binds the UDP socket to a specific endpoint.
     * @param endpoint The endpoint to bind the socket to.
     * @return true if the socket was successfully bound, false otherwise.
     */
    virtual bool bind(const boost::asio::ip::udp::endpoint& endpoint) = 0;

    /**
     * Sends data to a specific endpoint.
     * @param endpoint The endpoint to send the data to.
     * @param data The data to send, represented as a span of bytes.
     */
    virtual void send(const boost::asio::ip::udp::endpoint& endpoint, const std::span<const std::byte>& data) = 0;
};

} // namespace lux::net::base