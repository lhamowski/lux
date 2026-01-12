#include <lux/io/proc/process_factory.hpp>

#include <lux/io/proc/process.hpp>

namespace lux::proc {

process_factory::process_factory(boost::asio::any_io_executor executor) : executor_{executor}
{
}

lux::proc::base::process_ptr process_factory::create_process(const std::string& exe_path,
                                                             lux::proc::base::process_handler& handler)
{
    return std::make_unique<lux::proc::process>(executor_, handler, exe_path);
}

} // namespace lux::proc