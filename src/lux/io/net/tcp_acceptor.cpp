#include <lux/io/net/tcp_acceptor.hpp>
#include <lux/io/net/tcp_inbound_socket.hpp>
#include <lux/io/net/detail/utils.hpp>

#include <lux/support/assert.hpp>
#include <lux/support/move.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <memory>
#include <system_error>

namespace lux::net {

template <typename Derived>
class base_tcp_acceptor : public std::enable_shared_from_this<base_tcp_acceptor<Derived>>
{
public:
    std::error_code listen(const lux::net::base::endpoint& endpoint)
    {
        boost::system::error_code ec;
        acceptor_.open(boost::asio::ip::tcp::v4(), ec);
        if (ec)
        {
            return ec;
        }

        acceptor_.set_option(boost::asio::socket_base::reuse_address{config_.reuse_address}, ec);
        if (ec)
        {
            boost::system::error_code ignored_ec;
            acceptor_.close(ignored_ec);
            return ec;
        }

        const auto boost_ep = lux::net::to_boost_endpoint<boost::asio::ip::tcp>(endpoint);
        acceptor_.bind(boost_ep, ec);
        if (ec)
        {
            boost::system::error_code ignored_ec;
            acceptor_.close(ignored_ec);
            return ec;
        }

        acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec)
        {
            boost::system::error_code ignored_ec;
            acceptor_.close(ignored_ec);
            return ec;
        }

        accept();
        return {};
    }

    std::error_code close()
    {
        boost::system::error_code ec;
        acceptor_.cancel(ec); // first error is ignored

        ec = {};
        acceptor_.close(ec);

        return ec;
    }

    std::optional<lux::net::base::endpoint> local_endpoint() const
    {
        boost::system::error_code ec;
        const auto boost_ep = acceptor_.local_endpoint(ec);
        if (ec)
        {
            return std::nullopt;
        }

        return lux::net::from_boost_endpoint<boost::asio::ip::tcp>(boost_ep);
    }

    /**
     * Clears the handler.
     * This is used to prevent dangling pointers when the parent is destroyed but the impl is still alive (e.g., in
     * completion handlers).
     */
    void detach_external_references()
    {
        handler_ = nullptr;
    }

protected:
    std::shared_ptr<Derived> shared_from_base()
    {
        return std::static_pointer_cast<Derived>(this->shared_from_this());
    }

protected:
    void accept()
    {
        acceptor_.async_accept([self = this->shared_from_this()](const boost::system::error_code& ec,
                                                                 boost::asio::ip::tcp::socket socket) {
            self->on_accepted(ec, lux::move(socket));
        });
    }

private:
    explicit base_tcp_acceptor(boost::asio::any_io_executor exe,
                               lux::net::base::tcp_acceptor_handler& handler,
                               const lux::net::base::tcp_acceptor_config& config)
        : handler_{&handler}, config_{config}, acceptor_{exe}
    {
    }

private:
    friend Derived;

    Derived& derived()
    {
        return static_cast<Derived&>(*this);
    }

private:
    void on_accepted(const boost::system::error_code& ec, boost::asio::ip::tcp::socket&& socket)
    {
        if (ec)
        {
            if (ec == boost::asio::error::operation_aborted)
            {
                return;
            }

            if (handler_)
            {
                handler_->on_accept_error(ec);
            }

            // Even on error, continue accepting new connections
            accept();
            return;
        }

        derived().on_socket_accepted(lux::move(socket));
    }

protected:
    lux::net::base::tcp_acceptor_handler* handler_{nullptr};
    lux::net::base::tcp_acceptor_config config_{};

private:
    boost::asio::ip::tcp::acceptor acceptor_;
};

class tcp_acceptor::impl : public base_tcp_acceptor<tcp_acceptor::impl>
{
public:
    impl(boost::asio::any_io_executor exe,
         lux::net::base::tcp_acceptor_handler& handler,
         const lux::net::base::tcp_acceptor_config& config)
        : base_tcp_acceptor<tcp_acceptor::impl>(exe, handler, config)
    {
    }

public:
    void on_socket_accepted(boost::asio::ip::tcp::socket&& socket)
    {
        if (handler_)
        {
            const auto socket_config = lux::net::base::tcp_inbound_socket_config{
                .buffer = config_.socket_buffer,
            };

            handler_->on_accepted(std::make_unique<lux::net::tcp_inbound_socket>(lux::move(socket), socket_config));
        }

        // Continue accepting new connections
        accept();
    }
};

tcp_acceptor::tcp_acceptor(boost::asio::any_io_executor exe,
                           lux::net::base::tcp_acceptor_handler& handler,
                           const lux::net::base::tcp_acceptor_config& config)
    : impl_{std::make_shared<impl>(exe, handler, config)}
{
}

tcp_acceptor::~tcp_acceptor()
{
    LUX_ASSERT(impl_, "TCP acceptor implementation must not be null");
    impl_->detach_external_references();
}

std::error_code tcp_acceptor::listen(const lux::net::base::endpoint& endpoint)
{
    LUX_ASSERT(impl_, "TCP acceptor implementation must not be null");
    return impl_->listen(endpoint);
}

std::error_code tcp_acceptor::close()
{
    LUX_ASSERT(impl_, "TCP acceptor implementation must not be null");
    return impl_->close();
}

std::optional<lux::net::base::endpoint> tcp_acceptor::local_endpoint() const
{
    LUX_ASSERT(impl_, "TCP acceptor implementation must not be null");
    return impl_->local_endpoint();
}

class ssl_tcp_acceptor::impl : public base_tcp_acceptor<ssl_tcp_acceptor::impl>
{
public:
    impl(boost::asio::any_io_executor exe,
         lux::net::base::tcp_acceptor_handler& handler,
         const lux::net::base::tcp_acceptor_config& config,
         lux::net::base::ssl_context& ssl_context)
        : base_tcp_acceptor<ssl_tcp_acceptor::impl>(exe, handler, config), ssl_context_{ssl_context}
    {
    }

public:
    void on_socket_accepted(boost::asio::ip::tcp::socket&& socket)
    {
        // We want to call on_accepted only after the SSL handshake is complete.
        LUX_ASSERT(!temp_stream_.has_value(), "Temporary SSL stream must be empty before starting handshake");

        temp_stream_.emplace(lux::move(socket), ssl_context_);
        handshake();
    }

private:
    void handshake()
    {
        LUX_ASSERT(temp_stream_.has_value(), "Temporary SSL stream must be set before starting handshake");

        temp_stream_->async_handshake(
            boost::asio::ssl::stream_base::server,
            [self = shared_from_base()](const auto& ec) { self->on_handshake_completed(ec); });
    }

    void on_handshake_completed(const boost::system::error_code& ec)
    {
        if (ec)
        {
            if (ec == boost::asio::error::operation_aborted)
            {
                return;
            }

            if (handler_)
            {
                handler_->on_accept_error(ec);
            }

            return;
        }

        if (handler_)
        {
            LUX_ASSERT(temp_stream_.has_value(), "Temporary SSL stream must be set after handshake completion");

            const lux::net::base::tcp_inbound_socket_config socket_config = {
                .buffer = config_.socket_buffer,
            };

            auto ssl_socket = std::make_unique<lux::net::ssl_tcp_inbound_socket>(lux::move(*temp_stream_),
                                                                                 socket_config);
            
            // Reset the temporary stream optional after moving it to the inbound socket
            temp_stream_.reset();

            handler_->on_accepted(lux::move(ssl_socket));
        }

        // Continue accepting new connections
        accept();
    }

private:
    lux::net::base::ssl_context& ssl_context_;

    // Temporary SSL stream for handshake - after handshake, it will be moved to the inbound socket
    std::optional<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> temp_stream_;
};

ssl_tcp_acceptor::ssl_tcp_acceptor(boost::asio::any_io_executor exe,
                                   lux::net::base::tcp_acceptor_handler& handler,
                                   const lux::net::base::tcp_acceptor_config& config,
                                   lux::net::base::ssl_context& ssl_context)
    : impl_{std::make_shared<impl>(exe, handler, config, ssl_context)}
{
}

ssl_tcp_acceptor::~ssl_tcp_acceptor()
{
    LUX_ASSERT(impl_, "SSL TCP acceptor implementation must not be null");
    impl_->detach_external_references();
}

std::error_code ssl_tcp_acceptor::listen(const lux::net::base::endpoint& endpoint)
{
    LUX_ASSERT(impl_, "SSL TCP acceptor implementation must not be null");
    return impl_->listen(endpoint);
}

std::error_code ssl_tcp_acceptor::close()
{
    LUX_ASSERT(impl_, "SSL TCP acceptor implementation must not be null");
    return impl_->close();
}

std::optional<lux::net::base::endpoint> ssl_tcp_acceptor::local_endpoint() const
{
    LUX_ASSERT(impl_, "SSL TCP acceptor implementation must not be null");
    return impl_->local_endpoint();
}

} // namespace lux::net
