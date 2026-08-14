#include <windows.h>

extern "C" bool zeus_system_prefers_dark() {
    DWORD light = 1;
    DWORD size = sizeof(light);
    const LSTATUS status = RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme",
        RRF_RT_REG_DWORD,
        nullptr,
        &light,
        &size);
    return status == ERROR_SUCCESS && light == 0;
}

extern "C" const char* zeus_system_locale_tag() {
    static char locale[LOCALE_NAME_MAX_LENGTH * 3] = "en";
    wchar_t locale_name[LOCALE_NAME_MAX_LENGTH] = {};
    if (GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH) == 0) return locale;
    const int written = WideCharToMultiByte(
        CP_UTF8, 0, locale_name, -1, locale, static_cast<int>(sizeof(locale)), nullptr, nullptr);
    if (written == 0) locale[0] = '\0';
    return locale;
}
