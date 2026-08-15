#include "file_dialog.h"

#import <AppKit/AppKit.h>

namespace app::platform {

std::string choose_input_file() {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;
        panel.resolvesAliases = YES;
        if ([panel runModal] != NSModalResponseOK || panel.URL == nil) return {};
        const char* path = panel.URL.path.UTF8String;
        return path == nullptr ? std::string{} : std::string(path);
    }
}

std::string choose_export_file(const std::string& suggested_name) {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        panel.canCreateDirectories = YES;
        panel.nameFieldStringValue = [NSString stringWithUTF8String:suggested_name.c_str()];
        if ([panel runModal] != NSModalResponseOK || panel.URL == nil) return {};
        const char* path = panel.URL.path.UTF8String;
        return path == nullptr ? std::string{} : std::string(path);
    }
}

} // namespace app::platform
