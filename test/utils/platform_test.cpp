#include "test_case.hpp"

#include <lux/utils/platform.hpp>

#include <catch2/catch_all.hpp>

LUX_TEST_CASE("get_hostname", "returns non-empty hostname", "[utils][platform]")
{
    const auto hostname = lux::get_hostname();
    CHECK_FALSE(hostname.empty());
}

LUX_TEST_CASE("get_os", "returns expected operating system name", "[utils][platform]")
{
    const auto os = lux::get_os();
    CHECK_FALSE(os.empty());

#ifdef _WIN32
    CHECK(os == "Windows");
#elif __APPLE__
    CHECK(os == "macOS");
#elif __linux__
    CHECK(os == "Linux");
#endif
}