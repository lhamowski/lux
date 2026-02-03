#include <lux/io/proc/process.hpp>

#include <lux/support/assert.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/stdio.hpp>
#include <boost/system/error_code.hpp>

#if defined(BOOST_PROCESS_V2_WINDOWS)
#include <boost/process/v2/windows/creation_flags.hpp>
#else
#include <boost/process.hpp>
#endif

#include <cstddef>
#include <format>
#include <optional>

namespace {

constexpr std::size_t stdout_buffer_size = 8 * 1024; // 8 KB standard output read buffer size
constexpr std::size_t stderr_buffer_size = 1024;     // 1 KB standard error read buffer size
} // namespace

namespace lux::proc {

class process::impl : public std::enable_shared_from_this<impl>
{
public:
    impl(boost::asio::any_io_executor executor, lux::proc::base::process_handler& handler, const std::string& exe_path)
        : executor_{executor}, handler_{handler}, exe_path_{exe_path}, stdout_pipe_{executor}, stderr_pipe_{executor}
    {
        stdout_read_buffer_.resize(stdout_buffer_size);
        stderr_read_buffer_.resize(stderr_buffer_size);
    }

    ~impl()
    {
        if (process_.has_value())
        {
            boost::system::error_code ignored_ec;
            process_->terminate(ignored_ec);
        }
    }

public:
    lux::status start(const std::vector<std::string>& args)
    {
        namespace bp = boost::process::v2;

        try
        {
            process_.emplace(executor_,
                             exe_path_,
                             args,
                             bp::process_stdio{.in = {}, .out = stdout_pipe_, .err = stderr_pipe_}
#if defined(BOOST_PROCESS_V2_WINDOWS)
                             , bp::windows::create_new_process_group
#endif
            );
        }
        catch (const std::exception& ex)
        {
            return lux::err("Failed to start process (exe={}, err={})", exe_path_, ex.what());
        }

        read_stdout();
        read_stderr();

        process_->async_wait([self = this->shared_from_this()](const auto& ec, int exit_code) {

            boost::system::error_code ignored_ec;
            self->stdout_pipe_.close(ignored_ec);
            self->stderr_pipe_.close(ignored_ec);

            if (ec == boost::asio::error::operation_aborted)
            {
                return;
            }

            if (ec)
            {
                self->handler_.on_process_error(std::format("Error waiting for process exit (err={})", ec.message()));
            }
            else
            {
                self->on_process_exit(exit_code);
            }
        });

        return lux::ok();
    }

    void terminate()
    {
        namespace bp = boost::process::v2;
        
        if (!process_.has_value())
        {
            return;
        }

        boost::system::error_code ec;
        process_->terminate(ec);

        if (ec)
        {
            handler_.on_process_error(std::format("Error terminating process (err={})", ec.message()));
        }
    }

    bool is_running() const
    {
        if (!process_.has_value())
        {
            return false;
        }

        boost::system::error_code ec;
        return process_->running(ec);
    }

private:
    void read_stdout()
    {
        stdout_pipe_.async_read_some(
            boost::asio::buffer(stdout_read_buffer_),
            [self = this->shared_from_this()](const auto& ec, auto size) { self->on_stdout_read(ec, size); });
    }

    void read_stderr()
    {
        stderr_pipe_.async_read_some(
            boost::asio::buffer(stderr_read_buffer_),
            [self = this->shared_from_this()](const auto& ec, auto size) { self->on_stderr_read(ec, size); });
    }

private:
    void on_stdout_read(const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        if (ec == boost::asio::error::operation_aborted || ec == boost::asio::error::broken_pipe ||
            ec == boost::asio::error::eof)
        {
            return;
        }

        if (ec)
        {
            handler_.on_process_error(std::format("Error reading from stdout (err={})", ec.message()));
            return;
        }

        if (bytes_transferred > 0)
        {
            handler_.on_process_stdout(std::string_view{stdout_read_buffer_.data(), bytes_transferred});
        }

        read_stdout();
    }

    void on_stderr_read(const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        if (ec == boost::asio::error::operation_aborted || ec == boost::asio::error::broken_pipe ||
            ec == boost::asio::error::eof)
        {
            return;
        }

        if (ec)
        {
            handler_.on_process_error(std::format("Error reading from stderr (err={})", ec.message()));
            return;
        }

        if (bytes_transferred > 0)
        {
            handler_.on_process_stderr(std::string_view{stderr_read_buffer_.data(), bytes_transferred});
        }

        read_stderr();
    }

private:
    void on_process_exit(int exit_code)
    {
        process_.reset();
        handler_.on_process_exit(exit_code);
    }


private:
    boost::asio::any_io_executor executor_;
    lux::proc::base::process_handler& handler_;
    const std::string exe_path_;

private:
    boost::asio::readable_pipe stdout_pipe_;
    boost::asio::readable_pipe stderr_pipe_;
    mutable std::optional<boost::process::v2::process> process_;

private:
    std::vector<char> stdout_read_buffer_;
    std::vector<char> stderr_read_buffer_;
};

process::process(boost::asio::any_io_executor executor,
                 lux::proc::base::process_handler& handler,
                 const std::string& exe_path)
    : impl_{std::make_shared<impl>(executor, handler, exe_path)}
{
}

process::~process() = default;

lux::status process::start(const std::vector<std::string>& args)
{
    LUX_ASSERT(impl_, "Process implementation must not be null");
    return impl_->start(args);
}

void process::terminate()
{
    LUX_ASSERT(impl_, "Process implementation must not be null");
    impl_->terminate();
}

bool process::is_running() const
{
    LUX_ASSERT(impl_, "Process implementation must not be null");
    return impl_->is_running();
}

} // namespace lux::proc