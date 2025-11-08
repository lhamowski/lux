#include <lux/utils/stopwatch.hpp>

#include <catch2/catch_all.hpp>

#include <thread>

TEST_CASE("Stopwatch functionality", "[stopwatch][utils]")
{
    lux::stopwatch sw;

    SECTION("Reset works")
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto elapsed = sw.elapsed_ms();
        CHECK(elapsed.count() >= 50);
        sw.reset();
        CHECK(sw.elapsed_ms().count() < 50);
    }

    SECTION("Custom duration type works")
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto elapsed = sw.elapsed<std::chrono::microseconds>();
        CHECK(elapsed.count() >= 10000);
    }
}