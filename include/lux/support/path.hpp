#pragma once

#include <filesystem>
#include <string>

namespace lux {

std::filesystem::path app_data_path(const std::string& app_name);
void create_app_data_directory(const std::string& app_name);

} // namespace lux