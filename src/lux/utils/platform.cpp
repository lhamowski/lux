#include <lux/utils/platform.hpp>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/utsname.h>
#include <unistd.h>

#include <climits>

#ifndef HOST_NAME_MAX
#ifdef _POSIX_HOST_NAME_MAX
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#else
#define HOST_NAME_MAX 255
#endif
#endif
#endif

namespace lux {

std::string get_hostname()
{
#ifdef _WIN32
    char buffer[MAX_COMPUTERNAME_LENGTH + 1]{};
    DWORD size{sizeof(buffer)};
    if (GetComputerNameA(buffer, &size))
    {
        return std::string{buffer, size};
    }
    return {};
#else
    char buffer[HOST_NAME_MAX + 1]{};
    if (gethostname(buffer, sizeof(buffer)) == 0)
    {
        return std::string{buffer};
    }
    return {};
#endif
}

std::string get_os()
{
#ifdef _WIN32
    return "Windows";
#elif __APPLE__
    return "macOS";
#elif __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix__
    return "Unix";
#else
    return "Unknown";
#endif
}

} // namespace lux
