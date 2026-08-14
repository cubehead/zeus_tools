#pragma once

namespace app::fonts {

inline const char* ui() {
    return "PingFang SC";
}

inline const char* code() {
#if defined(_WIN32)
    return "Cascadia Mono";
#elif defined(__APPLE__)
    return "SF Mono";
#else
    return "monospace";
#endif
}

} // namespace app::fonts
