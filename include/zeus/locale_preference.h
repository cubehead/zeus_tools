#pragma once

namespace zeus {

enum class LocalePreference {
    System = 0,
    Chinese = 1,
    English = 2,
    TraditionalChinese = 3,
    Japanese = 4,
    Korean = 5,
    Spanish = 6,
    French = 7,
    German = 8,
    Portuguese = 9,
    Russian = 10,
};

LocalePreference load_locale_preference();
bool save_locale_preference(LocalePreference preference);

} // namespace zeus
