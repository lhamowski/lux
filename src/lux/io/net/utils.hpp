#pragma once

#include <lux/io/net/base/endpoint.hpp>

#include <boost/asio/ip/basic_endpoint.hpp>

namespace lux::net {

template <typename Protocol>
lux::net::base::endpoint from_boost_endpoint(const boost::asio::ip::basic_endpoint<Protocol>& ep)
{
    const auto address_uint = ep.address().to_v4().to_uint();
    const auto address_v4 = lux::net::base::address_v4{address_uint};
    return lux::net::base::endpoint{address_v4, ep.port()};
}

template <typename Protocol>
boost::asio::ip::basic_endpoint<Protocol> to_boost_endpoint(const lux::net::base::endpoint& ep)
{
    const auto address = boost::asio::ip::address_v4{ep.address().to_uint()};
    return {address, ep.port()};
}

} // namespace lux::net