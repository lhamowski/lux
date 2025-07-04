#pragma once

#ifdef _WIN32
// std::getenv may generate warnings about unsafe functions
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <lux/support/path.hpp>

#include <cstdlib>

namespace lux {

std::filesystem::path app_data_path(const std::string& app_name)
{
#ifdef _WIN32
    // On Windows, use the APPDATA environment variable
    if (const char* appdata = std::getenv("APPDATA"))
    {
        return std::filesystem::path(appdata) / app_name;
    }
    else
    {
        throw std::runtime_error("APPDATA environment variable is not set.");
    }
#elif __APPLE__
    // On macOS, use the Library/Application Support directory
    if (const char* home = std::getenv("HOME"))
    {
        return std::filesystem::path(home) / "Library" / "Application Support" / app_name;
    }
    else
    {
        throw std::runtime_error("HOME environment variable is not set.");
    }
#else
    // Check if XDG_DATA_HOME is set, otherwise use the default path
    if (const char* xdg_data_home = std::getenv("XDG_DATA_HOME"))
    {
        return std::filesystem::path(xdg_data_home) / app_name;
    }
    else
    {
        // If XDG_DATA_HOME is not set, use the default path
        if (const char* home = std::getenv("HOME"))
        {
            return std::filesystem::path(home) / ".local" / "share" / app_name;
        }
        else
        {
            throw std::runtime_error("HOME environment variable is not set.");
        }
    }
#endif
}

void create_app_data_directory(const std::string& app_name)
{
    const auto app_data_dir = app_data_path(app_name);
    if (!std::filesystem::exists(app_data_dir))
    {
        std::filesystem::create_directories(app_data_dir);
    }
}
} // namespace lux