#pragma once

#include <string>

extern "C" const char* zeus_system_locale_tag();

namespace zeus {

inline std::string system_locale_tag() {
    const char* value = zeus_system_locale_tag();
    return value == nullptr ? std::string{} : std::string(value);
}

} // namespace zeus
