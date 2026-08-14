#import <AppKit/AppKit.h>
#include <stdbool.h>
#include <string.h>

bool zeus_system_prefers_dark(void) {
    @autoreleasepool {
        NSAppearance* appearance = NSApp.effectiveAppearance;
        if (appearance == nil) appearance = NSAppearance.currentDrawingAppearance;
        NSString* match = [appearance bestMatchFromAppearancesWithNames:@[
            NSAppearanceNameAqua,
            NSAppearanceNameDarkAqua
        ]];
        return [match isEqualToString:NSAppearanceNameDarkAqua];
    }
}

const char* zeus_system_locale_tag(void) {
    static char locale[32] = "en";
    @autoreleasepool {
        NSString* language = NSLocale.preferredLanguages.firstObject;
        const char* value = language.UTF8String;
        if (value != NULL && *value != '\0') {
            strncpy(locale, value, sizeof(locale) - 1);
            locale[sizeof(locale) - 1] = '\0';
        }
        return locale;
    }
}
