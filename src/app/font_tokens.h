#pragma once

namespace app::fonts {

inline const char* ui() {
#if defined(_WIN32)
    return "Microsoft YaHei";
#elif defined(__APPLE__)
    return "PingFang SC";
#else
    return "sans-serif";
#endif
}

// Microsoft YaHei UI has visibly larger control glyphs than PingFang at the
// same nominal pixel size. Keep document typography unchanged and normalize
// only button labels so Windows controls retain the same visual density.
constexpr float button_size(float requested) {
#if defined(_WIN32)
    return requested * 0.88f;
#else
    return requested;
#endif
}

// The action strip can contain both format-specific and common actions. Its
// Windows controls need a denser footprint to remain inside the default window.
constexpr float action_width(float requested) {
#if defined(_WIN32)
    return requested * 0.82f;
#else
    return requested;
#endif
}

constexpr float action_gap() {
#if defined(_WIN32)
    return 5.0f;
#else
    return 8.0f;
#endif
}

template <typename Tokens>
Tokens control_tokens(Tokens tokens) {
#if defined(_WIN32)
    tokens.metrics.typography.body *= 0.88f;
    tokens.metrics.typography.option *= 0.88f;
    tokens.metrics.typography.hint *= 0.88f;
#endif
    return tokens;
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
