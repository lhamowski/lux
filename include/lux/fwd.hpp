#pragma once

namespace lux {
class logger_factory;
class logger_manager;
class logger;

class memory_arena;
} // namespace lux

namespace lux::net::base {
class socket_factory;

class tcp_acceptor;
class tcp_acceptor_handler;

class tcp_socket;
class tcp_socket_handler;

class udp_socket;
class udp_socket_handler;
} // namespace lux::net::base

namespace lux::net {
class udp_socket;
class socket_factory;
} // namespace lux::net

namespace lux::time::base {
class timer_factory;
class interval_timer;
} // namespace lux::time::base

namespace lux::time {
struct delayed_retry_config;

class delayed_retry_executor;
} // namespace lux::time
