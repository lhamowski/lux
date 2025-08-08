#pragma once

#include <lux/io/net/base/endpoint.hpp>

#include <optional>
#include <span>
#include <string_view>
#include <system_error>

namespace lux::net::base {

struct tcp_socket_config
{
    /**
     * Keep-alive settings
     * If true, enables TCP keep-alive to maintain the connection.
     */
    bool keep_alive{false};

    /**
     * Reconnect settings
     * If true, attempts to reconnect on disconnect.
     */
    bool reconnect_on_disconnect{true};

    /**
     * Maximum number of reconnection attempts.
     * If reconnect_on_disconnect is true, this specifies how many times to attempt reconnection.
     */
    std::size_t max_reconnect_attempts{5};

    /**
     * Memory arena configuration
     * Initial size of each item in the memory arena.
     */
    std::size_t memory_arena_initial_item_size{1024};

    /**
     * Initial number of items in the memory arena.
     * This is used to preallocate memory for socket operations.
     */
    std::size_t memory_arena_initial_item_count{4};
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
     * Opens the TCP socket using a host and port.
     * @param host The host endpoint to connect to.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code connect(const lux::net::base::host_endpoint& host_endpoint) = 0;

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

    /**
     * Gets the local endpoint of the socket.
     * @return The local endpoint if connected, otherwise std::nullopt.
     */
    virtual std::optional<lux::net::base::endpoint> local_endpoint() const = 0;

    /**
     * Gets the remote endpoint of the socket.
     * @return The remote endpoint if connected, otherwise std::nullopt.
     */
    virtual std::optional<lux::net::base::endpoint> remote_endpoint() const = 0;

protected:
    virtual ~tcp_socket() = default;
};

class tcp_socket_handler
{
public:
    /**
     * Called when a connection is established with a specific endpoint.
     * @param socket The socket that is now connected.
     */
    virtual void on_connected(lux::net::base::tcp_socket& socket) = 0;

    /**
     * Called when a connection is disconnected from a specific endpoint.
     * @param socket The socket that was disconnected.
     * @param ec The error code indicating the reason for disconnection.
     */
    virtual void on_disconnected(lux::net::base::tcp_socket& socket, const std::error_code& ec) = 0;

    /**
     * Called when data is received from a specific endpoint.
     * @param socket The socket that received the data.
     * @param data The received data, represented as a span of bytes.
     */
    virtual void on_data_read(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) = 0;

    /**
     * Called when data is successfully sent to a specific endpoint.
     * @param socket The socket that sent the data.
     * @param data The data that was sent, represented as a span of bytes.
     */
    virtual void on_data_sent(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) = 0;

protected:
    virtual ~tcp_socket_handler() = default;
};

} // namespace lux::net::base