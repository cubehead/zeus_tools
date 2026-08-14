#pragma once

namespace zeus {

enum class ThemePreference {
    System = 0,
    Light = 1,
    Dark = 2,
};

ThemePreference load_theme_preference();
bool save_theme_preference(ThemePreference preference);

} // namespace zeus
