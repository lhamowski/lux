#pragma once

namespace lux {

class logger_factory;
class logger_manager;
class logger;

class memory_arena;
class error_message;
} // namespace lux

namespace lux::net::base {

class endpoint;
class hostname_endpoint;

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

class socket_factory;
class tcp_socket;
class tcp_inbound_socket;
class udp_socket;

} // namespace lux::net

namespace lux::time::base {

class timer_factory;
class interval_timer;

} // namespace lux::time::base

namespace lux::time {

struct delayed_retry_config;

class delayed_retry_executor;

} // namespace lux::time
