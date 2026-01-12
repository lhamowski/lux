#include <chrono>
#include <iostream>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/signal_set.hpp>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        return 0;
    }

    const std::string command{argv[1]};

    if (command == "stdout")
    {
        std::cout << "test stdout message" << std::flush;
        return 0;
    }

    if (command == "stderr")
    {
        std::cerr << "test stderr message" << std::flush;
        return 0;
    }

    if (command == "both")
    {
        std::cout << "stdout line" << std::flush;
        std::cerr << "stderr line" << std::flush;
        return 0;
    }

    if (command == "exit_code")
    {
        if (argc > 2)
        {
            return std::stoi(argv[2]);
        }
        return 42;
    }

    if (command == "sleep")
    {
        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGTERM);

        int return_code = 0;
        signals.async_wait([&return_code, &io_context](const boost::system::error_code&, int signal_number) {
            return_code = signal_number;
            io_context.stop();
        });

        const std::chrono::seconds seconds{argc > 2 ? std::stoi(argv[2]) : 5};

        boost::asio::steady_timer timer{io_context, seconds};
        timer.async_wait([](const boost::system::error_code&) {});
        io_context.run();

        return return_code;
    }

    return 1;
}