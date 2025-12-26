#pragma once

#include <boost/asio/ssl/context.hpp>

namespace lux::net::base {

enum class ssl_mode
{
    client,
    server,
};

using ssl_context = boost::asio::ssl::context;

} // namespace lux::net::base