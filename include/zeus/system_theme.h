#pragma once

extern "C" bool zeus_system_prefers_dark();

namespace zeus {

inline bool system_prefers_dark() {
    return zeus_system_prefers_dark();
}

} // namespace zeus
