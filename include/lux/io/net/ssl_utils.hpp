#pragma once

#include <lux/io/net/base/ssl.hpp>

namespace lux::net {

void set_default_ssl_verify_paths(lux::net::base::ssl_context& ssl_context);

}
