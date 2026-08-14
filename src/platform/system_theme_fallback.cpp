#include <cstdlib>
#include <string>

extern "C" bool zeus_system_prefers_dark() {
    return false;
}

extern "C" const char* zeus_system_locale_tag() {
    static std::string locale = "en";
    const char* value = std::getenv("LC_ALL");
    if (value == nullptr || *value == '\0') value = std::getenv("LC_MESSAGES");
    if (value == nullptr || *value == '\0') value = std::getenv("LANG");
    if (value != nullptr && *value != '\0') locale = value;
    return locale.c_str();
}
