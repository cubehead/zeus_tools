if(NOT DEFINED SOURCE_DIR OR NOT IS_DIRECTORY "${SOURCE_DIR}")
    message(FATAL_ERROR "SOURCE_DIR must point to an EUI-NEO checkout")
endif()

function(zeus_replace_once relative_path before after)
    set(path "${SOURCE_DIR}/${relative_path}")
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "EUI accessibility patch target is missing: ${path}")
    endif()
    file(READ "${path}" contents)
    string(FIND "${contents}" "${after}" already_patched)
    if(NOT already_patched EQUAL -1)
        return()
    endif()
    string(FIND "${contents}" "${before}" original_found)
    if(original_found EQUAL -1)
        message(FATAL_ERROR "EUI accessibility patch no longer applies to: ${path}")
    endif()
    string(REPLACE "${before}" "${after}" contents "${contents}")
    file(WRITE "${path}" "${contents}")
endfunction()

set(header_before [=[void eui_set_native_child_window(void* parentWindow, void* childWindow, int enabled);
]=])
set(header_after [=[void eui_set_native_child_window(void* parentWindow, void* childWindow, int enabled);

enum {
    EUI_ACCESSIBILITY_BUTTON = 1,
    EUI_ACCESSIBILITY_POPUP_BUTTON = 2,
    EUI_ACCESSIBILITY_TEXT_AREA = 3,
    EUI_ACCESSIBILITY_TEXT_FIELD = 4
};

typedef struct eui_accessibility_element {
    const char* id;
    const char* label;
    const char* value;
    const char* help;
    int role;
    float x;
    float y;
    float width;
    float height;
    int enabled;
    int pressable;
} eui_accessibility_element;

void eui_set_accessibility_elements(
    const eui_accessibility_element* elements, int count, float logicalHeight);
int eui_take_accessibility_action(char* targetId, int capacity);
]=])
set(native_bridge_header_path "${SOURCE_DIR}/core/platform/native_bridge.h")
file(READ "${native_bridge_header_path}" native_bridge_header_contents)
string(FIND "${native_bridge_header_contents}" "typedef struct eui_accessibility_element" accessibility_header_present)
if(accessibility_header_present EQUAL -1)
    string(FIND "${native_bridge_header_contents}" "${header_before}" header_marker_found)
    if(header_marker_found EQUAL -1)
        message(FATAL_ERROR "EUI accessibility header insertion point no longer applies to: ${native_bridge_header_path}")
    endif()
    string(REPLACE "${header_before}" "${header_after}" native_bridge_header_contents "${native_bridge_header_contents}")
    file(WRITE "${native_bridge_header_path}" "${native_bridge_header_contents}")
endif()

set(apple_marker [=[
#else

void eui_set_application_icon_rgba]=])
set(apple_impl [=[

static char euiAccessibilityActionTarget[512];
static int euiAccessibilityAction = 0;
static NSString* euiAccessibilityFocusedId = nil;
static NSMutableDictionary* euiAccessibilityCache = nil;

static void eui_queue_accessibility_action(NSString* target, int action) {
    if (target == nil || [target length] == 0) return;
    @synchronized([NSApplication class]) {
        const char* utf8 = [target UTF8String];
        if (utf8 == NULL) return;
        strncpy(euiAccessibilityActionTarget, utf8, sizeof(euiAccessibilityActionTarget) - 1u);
        euiAccessibilityActionTarget[sizeof(euiAccessibilityActionTarget) - 1u] = '\0';
        euiAccessibilityAction = action;
        [euiAccessibilityFocusedId release];
        euiAccessibilityFocusedId = [target copy];
    }
}

@interface EUIAccessibilityProxy : NSAccessibilityElement {
    NSString* _euiTargetId;
    BOOL _euiPressable;
}
- (void)setEuiTargetId:(NSString*)targetId;
- (void)setEuiPressable:(BOOL)pressable;
@end

@implementation EUIAccessibilityProxy
- (void)dealloc {
    [_euiTargetId release];
    [super dealloc];
}
- (void)setEuiTargetId:(NSString*)targetId {
    if (_euiTargetId == targetId || [_euiTargetId isEqualToString:targetId]) return;
    [_euiTargetId release];
    _euiTargetId = [targetId copy];
}
- (void)setEuiPressable:(BOOL)pressable { _euiPressable = pressable; }
- (BOOL)isAccessibilityFocused {
    return _euiTargetId != nil && [_euiTargetId isEqualToString:euiAccessibilityFocusedId];
}
- (void)setAccessibilityFocused:(BOOL)focused {
    if (focused) eui_queue_accessibility_action(_euiTargetId, 1);
}
- (BOOL)accessibilityPerformPress {
    if (!_euiPressable) return NO;
    eui_queue_accessibility_action(_euiTargetId, 2);
    return YES;
}
@end

static NSString* eui_accessibility_string(const char* value) {
    if (value == NULL || value[0] == '\0') return @"";
    NSString* string = [NSString stringWithUTF8String:value];
    return string != nil ? string : @"";
}

void eui_set_accessibility_elements(
    const eui_accessibility_element* elements, int count, float logicalHeight) {
    @autoreleasepool {
        NSWindow* window = [NSApp keyWindow];
        NSView* content = [window contentView];
        if (window == nil || content == nil) return;
        if (euiAccessibilityCache == nil) {
            euiAccessibilityCache = [[NSMutableDictionary alloc] init];
        }
        NSMutableArray* children = [NSMutableArray arrayWithCapacity:(NSUInteger)(count > 0 ? count : 0)];
        NSMutableSet* activeIds = [NSMutableSet setWithCapacity:(NSUInteger)(count > 0 ? count : 0)];
        for (int index = 0; index < count; ++index) {
            const eui_accessibility_element* source = &elements[index];
            NSString* identifier = eui_accessibility_string(source->id);
            if ([identifier length] == 0) continue;
            EUIAccessibilityProxy* proxy = [euiAccessibilityCache objectForKey:identifier];
            if (proxy == nil) {
                proxy = [[EUIAccessibilityProxy alloc] init];
                [proxy setEuiTargetId:identifier];
                [euiAccessibilityCache setObject:proxy forKey:identifier];
                [proxy release];
            }
            NSString* role = NSAccessibilityButtonRole;
            if (source->role == EUI_ACCESSIBILITY_POPUP_BUTTON) role = NSAccessibilityPopUpButtonRole;
            else if (source->role == EUI_ACCESSIBILITY_TEXT_AREA) role = NSAccessibilityTextAreaRole;
            else if (source->role == EUI_ACCESSIBILITY_TEXT_FIELD) role = NSAccessibilityTextFieldRole;
            [proxy setAccessibilityRole:role];
            [proxy setAccessibilityLabel:eui_accessibility_string(source->label)];
            [proxy setAccessibilityValue:eui_accessibility_string(source->value)];
            [proxy setAccessibilityHelp:eui_accessibility_string(source->help)];
            [proxy setAccessibilityEnabled:source->enabled != 0];
            [proxy setAccessibilityParent:content];
            [proxy setEuiPressable:source->pressable != 0];
            NSRect local = NSMakeRect(source->x,
                                      logicalHeight - source->y - source->height,
                                      source->width, source->height);
            NSRect windowRect = [content convertRect:local toView:nil];
            [proxy setAccessibilityFrame:[window convertRectToScreen:windowRect]];
            [activeIds addObject:identifier];
            [children addObject:proxy];
        }
        for (NSString* identifier in [[euiAccessibilityCache allKeys] copy]) {
            if (![activeIds containsObject:identifier]) [euiAccessibilityCache removeObjectForKey:identifier];
        }
        [content setAccessibilityElement:NO];
        [content setAccessibilityChildren:children];
        NSAccessibilityPostNotification(content, NSAccessibilityLayoutChangedNotification);
    }
}

int eui_take_accessibility_action(char* targetId, int capacity) {
    if (targetId == NULL || capacity <= 0) return 0;
    @synchronized([NSApplication class]) {
        if (euiAccessibilityAction == 0) return 0;
        strncpy(targetId, euiAccessibilityActionTarget, (size_t)capacity - 1u);
        targetId[capacity - 1] = '\0';
        const int action = euiAccessibilityAction;
        euiAccessibilityAction = 0;
        euiAccessibilityActionTarget[0] = '\0';
        return action;
    }
}

#else

void eui_set_application_icon_rgba]=])
set(native_bridge_path "${SOURCE_DIR}/core/platform/native_bridge.c")
file(READ "${native_bridge_path}" native_bridge_contents)
string(FIND "${native_bridge_contents}" "@interface EUIAccessibilityProxy" accessibility_present)
if(accessibility_present EQUAL -1)
    string(FIND "${native_bridge_contents}" "${apple_marker}" apple_marker_found)
    if(apple_marker_found EQUAL -1)
        message(FATAL_ERROR "EUI accessibility insertion point no longer applies to: ${native_bridge_path}")
    endif()
    string(REPLACE "${apple_marker}" "${apple_impl}" native_bridge_contents "${native_bridge_contents}")
    file(WRITE "${native_bridge_path}" "${native_bridge_contents}")
endif()

set(window_before [=[        NSWindow* window = [NSApp keyWindow];
        NSView* content = [window contentView];]=])
set(window_after [=[        NSWindow* window = [NSApp keyWindow];
        if (window == nil) window = [NSApp mainWindow];
        if (window == nil && [[NSApp windows] count] > 0) window = [[NSApp windows] objectAtIndex:0];
        NSView* content = [window contentView];]=])
zeus_replace_once("core/platform/native_bridge.c" "${window_before}" "${window_after}")

set(fallback_before [=[void eui_set_native_child_window(void* parentWindow, void* childWindow, int enabled) {
    (void)parentWindow;
    (void)childWindow;
    (void)enabled;
}

#endif]=])
set(fallback_after [=[void eui_set_native_child_window(void* parentWindow, void* childWindow, int enabled) {
    (void)parentWindow;
    (void)childWindow;
    (void)enabled;
}

void eui_set_accessibility_elements(
    const eui_accessibility_element* elements, int count, float logicalHeight) {
    (void)elements;
    (void)count;
    (void)logicalHeight;
}

int eui_take_accessibility_action(char* targetId, int capacity) {
    (void)targetId;
    (void)capacity;
    return 0;
}

#endif]=])
zeus_replace_once("core/platform/native_bridge.c" "${fallback_before}" "${fallback_after}")

set(runtime_before [=[#pragma once

namespace core::dsl {]=])
set(runtime_after [=[#pragma once

#include "core/platform/native_bridge.h"

namespace core::dsl {]=])
zeus_replace_once("core/runtime/runtime_lifecycle.h" "${runtime_before}" "${runtime_after}")

set(update_before [=[    syncScrollStateBindings();
    if (scrollEvent.active()) {]=])
set(update_after [=[    char accessibilityTarget[512] = {};
    const int accessibilityAction = eui_take_accessibility_action(
        accessibilityTarget, static_cast<int>(sizeof(accessibilityTarget)));
    if (accessibilityAction != 0 && accessibilityTarget[0] != '\0') {
        const std::string targetId(accessibilityTarget);
        setFocusedId(targetId);
        if (accessibilityAction == 2) {
            if (const Element* element = ui_.find(targetId)) {
                if (!element->disabled && element->onClick) element->onClick();
            }
        }
    }

    syncScrollStateBindings();
    if (scrollEvent.active()) {]=])
zeus_replace_once("core/runtime/runtime_lifecycle.h" "${update_before}" "${update_after}")

zeus_replace_once(
    "core/platform/native_bridge.h"
    "    EUI_ACCESSIBILITY_TEXT_FIELD = 4\n};"
    "    EUI_ACCESSIBILITY_TEXT_FIELD = 4,\n    EUI_ACCESSIBILITY_STATIC_TEXT = 5\n};")

zeus_replace_once(
    "core/platform/native_bridge.c"
    "            else if (source->role == EUI_ACCESSIBILITY_TEXT_FIELD) role = NSAccessibilityTextFieldRole;"
    "            else if (source->role == EUI_ACCESSIBILITY_TEXT_FIELD) role = NSAccessibilityTextFieldRole;\n            else if (source->role == EUI_ACCESSIBILITY_STATIC_TEXT) role = NSAccessibilityStaticTextRole;")

# Preserve VoiceOver's current item while text is recomposed. Only structural
# or geometric changes announce a layout update; content changes announce value.
zeus_replace_once(
    "core/platform/native_bridge.c"
    "static NSMutableDictionary* euiAccessibilityCache = nil;"
    "static NSMutableDictionary* euiAccessibilityCache = nil;\nstatic NSArray* euiAccessibilityChildIds = nil;")

zeus_replace_once(
    "core/platform/native_bridge.c"
    "        NSMutableArray* children = [NSMutableArray arrayWithCapacity:(NSUInteger)(count > 0 ? count : 0)];\n        NSMutableSet* activeIds"
    "        NSMutableArray* children = [NSMutableArray arrayWithCapacity:(NSUInteger)(count > 0 ? count : 0)];\n        NSMutableArray* orderedIds = [NSMutableArray arrayWithCapacity:(NSUInteger)(count > 0 ? count : 0)];\n        BOOL layoutChanged = NO;\n        NSMutableSet* activeIds")

zeus_replace_once(
    "core/platform/native_bridge.c"
    "            EUIAccessibilityProxy* proxy = [euiAccessibilityCache objectForKey:identifier];\n            if (proxy == nil) {"
    "            EUIAccessibilityProxy* proxy = [euiAccessibilityCache objectForKey:identifier];\n            const BOOL newProxy = proxy == nil;\n            if (newProxy) {")

zeus_replace_once(
    "core/platform/native_bridge.c"
    "            [proxy setAccessibilityLabel:eui_accessibility_string(source->label)];\n            [proxy setAccessibilityValue:eui_accessibility_string(source->value)];"
    "            [proxy setAccessibilityLabel:eui_accessibility_string(source->label)];\n            NSString* newValue = eui_accessibility_string(source->value);\n            NSString* oldValue = [proxy accessibilityValue];\n            const BOOL valueChanged = !newProxy && ![(oldValue != nil ? oldValue : @\"\") isEqualToString:newValue];\n            [proxy setAccessibilityValue:newValue];")

zeus_replace_once(
    "core/platform/native_bridge.c"
    "            NSRect windowRect = [content convertRect:local toView:nil];\n            [proxy setAccessibilityFrame:[window convertRectToScreen:windowRect]];\n            [activeIds addObject:identifier];\n            [children addObject:proxy];"
    "            NSRect windowRect = [content convertRect:local toView:nil];\n            NSRect screenFrame = [window convertRectToScreen:windowRect];\n            if (newProxy || !NSEqualRects([proxy accessibilityFrame], screenFrame)) layoutChanged = YES;\n            [proxy setAccessibilityFrame:screenFrame];\n            [activeIds addObject:identifier];\n            [orderedIds addObject:identifier];\n            [children addObject:proxy];\n            if (valueChanged) NSAccessibilityPostNotification(proxy, NSAccessibilityValueChangedNotification);")

zeus_replace_once(
    "core/platform/native_bridge.c"
    "        for (NSString* identifier in [[euiAccessibilityCache allKeys] copy]) {\n            if (![activeIds containsObject:identifier]) [euiAccessibilityCache removeObjectForKey:identifier];\n        }\n        [content setAccessibilityElement:NO];\n        [content setAccessibilityChildren:children];\n        NSAccessibilityPostNotification(content, NSAccessibilityLayoutChangedNotification);"
    "        for (NSString* identifier in [euiAccessibilityCache allKeys]) {\n            if (![activeIds containsObject:identifier]) {\n                [euiAccessibilityCache removeObjectForKey:identifier];\n                layoutChanged = YES;\n            }\n        }\n        if (euiAccessibilityChildIds == nil || ![euiAccessibilityChildIds isEqualToArray:orderedIds]) {\n            layoutChanged = YES;\n            [euiAccessibilityChildIds release];\n            euiAccessibilityChildIds = [orderedIds copy];\n        }\n        [content setAccessibilityElement:NO];\n        [content setAccessibilityChildren:children];\n        if (layoutChanged) NSAccessibilityPostNotification(content, NSAccessibilityLayoutChangedNotification);")
