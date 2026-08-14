#import <CoreFoundation/CoreFoundation.h>
#import <CoreText/CoreText.h>

#include <string.h>

const char* zeus_system_ui_font_path_native(void) {
    static char path[4096];
    static int resolved = 0;
    if (resolved) return path;
    resolved = 1;

    CTFontDescriptorRef descriptor = CTFontDescriptorCreateWithNameAndSize(
        CFSTR("PingFangSC-Regular"), 14.0);
    if (descriptor == NULL) return path;
    CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute(
        descriptor, kCTFontURLAttribute);
    if (url != NULL) {
        CFStringRef value = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
        if (value != NULL) {
            CFStringGetCString(value, path, sizeof(path), kCFStringEncodingUTF8);
            CFRelease(value);
        }
        CFRelease(url);
    }
    CFRelease(descriptor);
    return path;
}
