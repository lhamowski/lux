#pragma once

#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/time/base/retry_policy.hpp>

#include <boost/asio/ssl/context.hpp>

#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <system_error>

namespace lux::net::base {

enum class ssl_mode
{
    client,
    server,
};

using ssl_context = boost::asio::ssl::context;

struct tcp_socket_config
{
    /**
     * Keep-alive settings
     * If true, enables TCP keep-alive to maintain the connection.
     */
    bool keep_alive{false};

    struct reconnect_config
    {
        /**
         * If true, enables automatic reconnection attempts.
         * If false, the socket will not attempt to reconnect after disconnection.
         */
        bool enabled{true};

        /**
         * Retry policy configuration for reconnection attempts.
         */
        lux::time::base::retry_policy reconnect_policy{
            .strategy = lux::time::base::retry_policy::backoff_strategy::exponential_backoff,
            .max_attempts = std::nullopt,
            .base_delay = std::chrono::milliseconds{1000},
            .max_delay = std::chrono::milliseconds{30000}};

    } reconnect{};

    struct buffer_config
    {
        /**
         * Size of each allocated buffer chunk in bytes.
         */
        std::size_t initial_send_chunk_size{1024};

        /**
         * Number of buffer chunks to preallocate.
         */
        std::size_t initial_send_chunk_count{4};

        /**
         * Size of read buffer to preallocate for reading data.
         */
        std::size_t read_buffer_size{8 * 1024}; // 8 KB

    } buffer{};
};

class tcp_socket
{
public:
    virtual ~tcp_socket() = default;

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
    virtual std::error_code send(const std::span<const std::byte>& data) = 0;

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
};

using tcp_socket_ptr = std::unique_ptr<tcp_socket>;

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
     * @param will_reconnect True if the socket will attempt to reconnect, false otherwise.
     */
    virtual void
        on_disconnected(lux::net::base::tcp_socket& socket, const std::error_code& ec, bool will_reconnect) = 0;

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