#include "zeus/theme_preference.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace zeus {
namespace {

std::filesystem::path settings_path() {
#ifdef _WIN32
    const char* base = std::getenv("APPDATA");
    if (base == nullptr || *base == '\0') return {};
    return std::filesystem::path(base) / "Zeus Tools" / "settings.conf";
#elif defined(__APPLE__)
    const char* base = std::getenv("HOME");
    if (base == nullptr || *base == '\0') return {};
    return std::filesystem::path(base) / "Library" / "Application Support" /
           "Zeus Tools" / "settings.conf";
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg != nullptr && *xdg != '\0') {
        return std::filesystem::path(xdg) / "zeus-tools" / "settings.conf";
    }
    const char* base = std::getenv("HOME");
    if (base == nullptr || *base == '\0') return {};
    return std::filesystem::path(base) / ".config" / "zeus-tools" / "settings.conf";
#endif
}

} // namespace

ThemePreference load_theme_preference() {
    const auto path = settings_path();
    if (path.empty()) return ThemePreference::System;
    std::ifstream input(path);
    std::string value;
    if (!std::getline(input, value)) return ThemePreference::System;
    if (value == "light") return ThemePreference::Light;
    if (value == "dark") return ThemePreference::Dark;
    return ThemePreference::System;
}

bool save_theme_preference(ThemePreference preference) {
    const auto path = settings_path();
    if (path.empty()) return false;
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) return false;
    std::ofstream output(path, std::ios::trunc);
    if (!output) return false;
    switch (preference) {
    case ThemePreference::Light: output << "light\n"; break;
    case ThemePreference::Dark: output << "dark\n"; break;
    case ThemePreference::System: output << "system\n"; break;
    }
    return output.good();
}

} // namespace zeus
