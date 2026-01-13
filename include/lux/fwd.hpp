#pragma once

namespace lux {

class logger_factory;
class logger_manager;
class logger;

class memory_arena;
class error_message;
} // namespace lux

namespace lux::crypto {

struct ed25519_private_key;
struct ed25519_public_key;

struct subject_info;

} // namespace lux::crypto

namespace lux::net::base {

class endpoint;
class hostname_endpoint;

enum class http_method;
enum class http_status;

class http_factory;
class http_request;
class http_response;

class http_server;
class http_server_handler;
struct http_server_config;

class http_client;
struct http_client_config;

class socket_factory;

class tcp_acceptor;
class tcp_acceptor_handler;
struct tcp_acceptor_config;

class tcp_socket;
class tcp_socket_handler;
struct tcp_socket_config;

class tcp_inbound_socket;
class tcp_inbound_socket_handler;
struct tcp_inbound_socket_config;

class udp_socket;
class udp_socket_handler;

} // namespace lux::net::base

namespace lux::net {

class http_router;
class http_server_app;
class socket_factory;
class tcp_socket;
class tcp_inbound_socket;
class udp_socket;

} // namespace lux::net

namespace lux::proc::base {

class process_factory;
class process;

} // namespace lux::proc::base

namespace lux::proc {

class process_factory;
class process;

} // namespace lux::proc

namespace lux::time::base {

class timer_factory;
class interval_timer;

} // namespace lux::time::base

namespace lux::time {

struct delayed_retry_config;

class delayed_retry_executor;

} // namespace lux::time
