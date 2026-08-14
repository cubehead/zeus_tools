#include "zeus/locale_preference.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace zeus {
namespace {

std::filesystem::path locale_path() {
#ifdef _WIN32
    const char* base = std::getenv("APPDATA");
    if (base == nullptr || *base == '\0') return {};
    return std::filesystem::path(base) / "Zeus Tools" / "locale.conf";
#elif defined(__APPLE__)
    const char* base = std::getenv("HOME");
    if (base == nullptr || *base == '\0') return {};
    return std::filesystem::path(base) / "Library" / "Application Support" /
           "Zeus Tools" / "locale.conf";
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg != nullptr && *xdg != '\0') {
        return std::filesystem::path(xdg) / "zeus-tools" / "locale.conf";
    }
    const char* base = std::getenv("HOME");
    if (base == nullptr || *base == '\0') return {};
    return std::filesystem::path(base) / ".config" / "zeus-tools" / "locale.conf";
#endif
}

} // namespace

LocalePreference load_locale_preference() {
    const auto path = locale_path();
    if (path.empty()) return LocalePreference::System;
    std::ifstream input(path);
    std::string value;
    if (!std::getline(input, value)) return LocalePreference::System;
    if (value == "zh-CN") return LocalePreference::Chinese;
    if (value == "en") return LocalePreference::English;
    if (value == "zh-TW") return LocalePreference::TraditionalChinese;
    if (value == "ja") return LocalePreference::Japanese;
    if (value == "ko") return LocalePreference::Korean;
    if (value == "es") return LocalePreference::Spanish;
    if (value == "fr") return LocalePreference::French;
    if (value == "de") return LocalePreference::German;
    if (value == "pt") return LocalePreference::Portuguese;
    if (value == "ru") return LocalePreference::Russian;
    return LocalePreference::System;
}

bool save_locale_preference(LocalePreference preference) {
    const auto path = locale_path();
    if (path.empty()) return false;
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) return false;
    std::ofstream output(path, std::ios::trunc);
    if (!output) return false;
    switch (preference) {
    case LocalePreference::Chinese: output << "zh-CN\n"; break;
    case LocalePreference::English: output << "en\n"; break;
    case LocalePreference::TraditionalChinese: output << "zh-TW\n"; break;
    case LocalePreference::Japanese: output << "ja\n"; break;
    case LocalePreference::Korean: output << "ko\n"; break;
    case LocalePreference::Spanish: output << "es\n"; break;
    case LocalePreference::French: output << "fr\n"; break;
    case LocalePreference::German: output << "de\n"; break;
    case LocalePreference::Portuguese: output << "pt\n"; break;
    case LocalePreference::Russian: output << "ru\n"; break;
    case LocalePreference::System: output << "system\n"; break;
    }
    return output.good();
}

} // namespace zeus
