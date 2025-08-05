#pragma once

#include <lux/net/base/endpoint.hpp>

#include <span>
#include <system_error>

namespace lux::net::base {

class tcp_socket_handler
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
     * Called when a connection is established with a specific endpoint.
     * @param endpoint The endpoint to which the connection was established.
     */
    virtual void on_connected(const lux::net::base::endpoint& endpoint) = 0;

    /**
     * Called when a connection is disconnected from a specific endpoint.
     * @param endpoint The endpoint from which the connection was disconnected.
     */
    virtual void on_disconnected(const lux::net::base::endpoint& endpoint) = 0;

protected:
    virtual  ~tcp_socket_handler() = default;
};

class tcp_socket
{
public:
    /**
     * Opens the TCP socket.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code connect(const lux::net::base::endpoint& endpoint) = 0;
    
    /**
     * Closes the TCP socket.
     * @param send_pending If true, sends any pending data before closing.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code disconnect(bool send_pending) = 0;
    
    /**
     * Sends data to the connected endpoint.
     * @param data The data to send, represented as a span of bytes.
     */
    virtual void send(const std::span<const std::byte>& data) = 0;

    /**
     * Checks if the socket is currently connected.
     * @return true if the socket is connected, false otherwise.
     */
    virtual bool is_connected() const = 0;

protected:
    virtual ~tcp_socket() = default;
};

} // namespace lux::net