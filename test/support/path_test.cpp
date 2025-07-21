#include <lux/support/path.hpp>

#include <lux/support/finally.hpp>

#include <catch2/catch_all.hpp>

#include <filesystem>
#include <string>
#include <optional>

TEST_CASE("Create app data directory", "[path][support]")
{
    SECTION("Create app data directory")
    {
        const std::string app_name = "TestApp";

        std::optional<std::filesystem::path> app_data_path;
        REQUIRE_NOTHROW(app_data_path.emplace(lux::app_data_path(app_name)));
        REQUIRE(app_data_path.has_value());

        // Ensure the directory does not exist before creating it
        std::filesystem::remove_all(*app_data_path);
        LUX_FINALLY({ std::filesystem::remove_all(*app_data_path); });

        REQUIRE_NOTHROW(lux::create_app_data_directory(app_name));
        CHECK(std::filesystem::exists(lux::app_data_path(app_name)));
    }
}
