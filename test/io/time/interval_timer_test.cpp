#include <lux/io/time/interval_timer.hpp>

#include <catch2/catch_all.hpp>

#include <chrono>
#include <cstddef>

TEST_CASE("interval_timer: scheduling and cancellation", "[io][time]")
{
    SECTION("Interval timer should schedule once correctly")
    {
        boost::asio::io_context io_context;
        lux::time::interval_timer timer{io_context.get_executor()};

        timer.schedule(std::chrono::milliseconds{10});

        bool called = false;
        timer.set_handler([&called, &io_context] {
            called = true;
            io_context.stop();
        });

        io_context.run_for(std::chrono::milliseconds{100});
        CHECK(called);
    }

    SECTION("Interval timer should schedule periodic correctly")
    {
        boost::asio::io_context io_context;
        lux::time::interval_timer timer{io_context.get_executor()};

        std::size_t called_count = 0;
        timer.set_handler([&called_count, &io_context] {
            ++called_count;
            if (called_count == 3)
            {
                io_context.stop();
            }
        });

        timer.schedule_periodic(std::chrono::milliseconds{5});
        io_context.run_for(std::chrono::milliseconds{100});

        CHECK(called_count == 3);
    }

    SECTION("Interval timer should cancel correctly")
    {
        boost::asio::io_context io_context;
        lux::time::interval_timer timer{io_context.get_executor()};

        std::size_t called_count = 0;
        timer.set_handler([&called_count, &timer] {
            ++called_count;
            timer.cancel();
        });

        timer.schedule_periodic(std::chrono::milliseconds{10});

        io_context.run_for(std::chrono::milliseconds{100});
        CHECK(called_count == 1);
    }

    SECTION("Interval timer should handle multiple schedules")
    {
        boost::asio::io_context io_context;
        lux::time::interval_timer timer{io_context.get_executor()};

        std::size_t called_count = 0;
        timer.set_handler([&called_count] { ++called_count; });
        timer.schedule(std::chrono::milliseconds{10});
        timer.schedule_periodic(std::chrono::milliseconds{20});
        timer.schedule(std::chrono::milliseconds{10000});

        io_context.run_for(std::chrono::milliseconds{100});
        CHECK(called_count == 0); // No calls yet, first schedule is in the future (10000 ms)

        // Reschedule and check again
        timer.schedule(std::chrono::milliseconds{10});

        io_context.restart();
        io_context.run_for(std::chrono::milliseconds{100});
        CHECK(called_count == 1);
    }

    SECTION("Interval timer should handle cancellation before first call")
    {
        boost::asio::io_context io_context;
        lux::time::interval_timer timer{io_context.get_executor()};

        std::size_t called_count = 0;
        timer.set_handler([&called_count] { ++called_count; });
        timer.schedule(std::chrono::milliseconds{10});
        timer.cancel(); // Cancel before the timer expires
        timer.cancel(); // Cancel again to ensure no double cancellation, should be a no-op

        io_context.run_for(std::chrono::milliseconds{20});
        CHECK(called_count == 0);
    }

    SECTION("Interval timer should handle empty handler")
    {
        boost::asio::io_context io_context;
        lux::time::interval_timer timer{io_context.get_executor()};
        timer.schedule(std::chrono::milliseconds{10});
        io_context.run_for(std::chrono::milliseconds{20});
    }
}