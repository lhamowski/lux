#pragma once

#include <lux/net/base/endpoint.hpp>

#include <span>
#include <system_error>

namespace lux::net::base {

class udp_socket_handler
{
public:
    /**
     * Called when data is received from a specific endpoint.
     * @param endpoint The endpoint from which the data was received.
     * @param data The received data, represented as a span of bytes.
     */
    virtual void on_data_read(const lux::net::base::endpoint& endpoint, const std::span<const std::byte>& data) = 0;

    /**
     * Called when data is successfully sent to a specific endpoint.
     * @param endpoint The endpoint to which the data was sent.
     * @param data The data that was sent, represented as a span of bytes.
     */
    virtual void on_data_sent(const lux::net::base::endpoint& endpoint, const std::span<const std::byte>& data) = 0;

    /**
     * Called when there is an error while reading data from a specific endpoint.
     * @param endpoint The endpoint from which the data was being read.
     * @param ec The error code representing the read error.
     */
    virtual void on_read_error(const lux::net::base::endpoint& endpoint, const std::error_code& ec) = 0;

    /**
     * Called when there is an error while sending data to a specific endpoint.
     * @param endpoint The endpoint to which the data was being sent.
     * @param data The data that was attempted to be sent, represented as a span of bytes.
     * @param ec The error code representing the send error.
     */
    virtual void on_send_error(const lux::net::base::endpoint& endpoint,
                               const std::span<const std::byte>& data,
                               const std::error_code& ec) = 0;

protected:
    virtual ~udp_socket_handler() = default;
};

struct udp_socket_config
{
    std::size_t memory_arena_initial_item_size = 1024; // Initial size of each item in the memory arena
    std::size_t memory_arena_initial_item_count = 4;   // Initial number of items in the memory arena
};

class udp_socket
{
public:
    virtual ~udp_socket() = default;

public:
    /**
     * Opens the UDP socket and starts listening for incoming data.
     *
     * @return true if the socket was successfully opened, false otherwise.
     */
    virtual std::error_code open() = 0;

    /**
     * Closes the UDP socket.
     *
     * @param send_pending_data If true, any pending data will be sent before closing the socket.
     * @return true if the socket was successfully closed, false otherwise.
     */
    virtual std::error_code close(bool send_pending_data) = 0;

    /**
     * Binds the UDP socket to a specific endpoint.
     * @param endpoint The endpoint to bind the socket to.
     * @return true if the socket was successfully bound, false otherwise.
     */
    virtual std::error_code bind(const lux::net::base::endpoint& endpoint) = 0;

    /**
     * Sends data to a specific endpoint.
     * @param endpoint The endpoint to send the data to.
     * @param data The data to send, represented as a span of bytes.
     */
    virtual void send(const lux::net::base::endpoint& endpoint, const std::span<const std::byte>& data) = 0;
};

} // namespace lux::net::base