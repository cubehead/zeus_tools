if(NOT DEFINED SOURCE_DIR OR NOT IS_DIRECTORY "${SOURCE_DIR}")
    message(FATAL_ERROR "SOURCE_DIR must point to an EUI-NEO checkout")
endif()

function(zeus_replace_once relative_path before after)
    set(path "${SOURCE_DIR}/${relative_path}")
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "EUI keyboard patch target is missing: ${path}")
    endif()
    file(READ "${path}" contents)
    string(FIND "${contents}" "${after}" already_patched)
    if(NOT already_patched EQUAL -1)
        return()
    endif()
    string(FIND "${contents}" "${before}" original_found)
    if(original_found EQUAL -1)
        message(FATAL_ERROR "EUI keyboard patch no longer applies to: ${path}")
    endif()
    string(REPLACE "${before}" "${after}" contents "${contents}")
    file(WRITE "${path}" "${contents}")
endfunction()

function(zeus_replace_if_present relative_path before after)
    set(path "${SOURCE_DIR}/${relative_path}")
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "EUI keyboard patch target is missing: ${path}")
    endif()
    file(READ "${path}" contents)
    string(FIND "${contents}" "${after}" already_patched)
    if(NOT already_patched EQUAL -1)
        return()
    endif()
    string(FIND "${contents}" "${before}" original_found)
    if(original_found EQUAL -1)
        return()
    endif()
    string(REPLACE "${before}" "${after}" contents "${contents}")
    file(WRITE "${path}" "${contents}")
endfunction()

zeus_replace_once(
    "core/input/input_types.h"
    "    bool enter = false;\n    bool left = false;"
    "    bool enter = false;\n    bool tab = false;\n    bool space = false;\n    bool left = false;")

zeus_replace_once(
    "core/input/input_types.h"
    "backspace || del || enter ||\n               left"
    "backspace || del || enter || tab || space ||\n               left")

zeus_replace_once(
    "core/input/input_types.h"
    "    Enter,\n    Left,"
    "    Enter,\n    Tab,\n    Space,\n    Left,")

zeus_replace_once(
    "core/input/input_state.h"
    "    bool enter = false;\n    bool left = false;"
    "    bool enter = false;\n    bool tab = false;\n    bool space = false;\n    bool left = false;")

zeus_replace_once(
    "core/input/input_state.h"
    "    case InputKey::Enter: queue.enter = true; break;\n    case InputKey::Left:"
    "    case InputKey::Enter: queue.enter = true; break;\n    case InputKey::Tab: queue.tab = true; break;\n    case InputKey::Space: queue.space = true; break;\n    case InputKey::Left:")

zeus_replace_once(
    "core/input/input_state.h"
    "        case GLFW_KEY_KP_ENTER: queueKeyInput(currentWindow, InputKey::Enter, ctrl, shift); break;\n        case GLFW_KEY_LEFT:"
    "        case GLFW_KEY_KP_ENTER: queueKeyInput(currentWindow, InputKey::Enter, ctrl, shift); break;\n        case GLFW_KEY_TAB: queueKeyInput(currentWindow, InputKey::Tab, ctrl, shift); break;\n        case GLFW_KEY_SPACE: queueKeyInput(currentWindow, InputKey::Space, ctrl, shift); break;\n        case GLFW_KEY_LEFT:")

zeus_replace_once(
    "core/input/input_state.h"
    "    keyboard.enter = queue.enter;\n    keyboard.left = queue.left;"
    "    keyboard.enter = queue.enter;\n    keyboard.tab = queue.tab;\n    keyboard.space = queue.space;\n    keyboard.left = queue.left;")

zeus_replace_once(
    "core/app/sdl2_app_main.cpp"
    "    case SDLK_KP_ENTER: mapped = core::InputKey::Enter; return true;\n    case SDLK_LEFT:"
    "    case SDLK_KP_ENTER: mapped = core::InputKey::Enter; return true;\n    case SDLK_TAB: mapped = core::InputKey::Tab; return true;\n    case SDLK_SPACE: mapped = core::InputKey::Space; return true;\n    case SDLK_LEFT:")

zeus_replace_once(
    "core/dsl_runtime.h"
    "    void setFocusedId(const std::string& id);\n\n    void updateScroll"
    "    void setFocusedId(const std::string& id);\n\n    void focusNext(bool reverse);\n\n    void updateScroll")

zeus_replace_once(
    "core/runtime/runtime_input.h"
    "inline void Runtime::updateScroll(const ScrollEvent& event, const std::string& targetId) {"
    "inline void Runtime::focusNext(bool reverse) {\n    std::vector<std::string> focusableIds;\n    std::function<void(const Element&)> collect = [&](const Element& element) {\n        if (element.focusable && !element.disabled && !isElementInDisabledTree(element.id)) {\n            focusableIds.push_back(element.id);\n        }\n        for (const Element* child : orderedElements(element)) {\n            collect(*child);\n        }\n    };\n    for (const Element* root : orderedElements(ui_)) {\n        collect(*root);\n    }\n    if (focusableIds.empty()) {\n        setFocusedId({});\n        return;\n    }\n    const auto current = std::find(focusableIds.begin(), focusableIds.end(), focusedId_);\n    std::size_t index = 0;\n    if (current == focusableIds.end()) {\n        index = reverse ? focusableIds.size() - 1 : 0;\n    } else {\n        const std::size_t position = static_cast<std::size_t>(current - focusableIds.begin());\n        index = reverse\n            ? (position + focusableIds.size() - 1) % focusableIds.size()\n            : (position + 1) % focusableIds.size();\n    }\n    setFocusedId(focusableIds[index]);\n}\n\ninline void Runtime::updateScroll(const ScrollEvent& event, const std::string& targetId) {")

zeus_replace_once(
    "core/runtime/runtime_lifecycle.h"
    "    if (keyboardEvent.find && !ui_.findShortcutTargetId().empty()) {"
    "    if (keyboardEvent.tab && !keyboardEvent.composing) {\n        focusNext(keyboardEvent.shift);\n        keyboardEvent.tab = false;\n    }\n    if (keyboardEvent.find && !ui_.findShortcutTargetId().empty()) {")

zeus_replace_once(
    "components/button.h"
    "        shadow = theme::buttonShadow(tokens);\n        radius ="
    "        shadow = theme::buttonShadow(tokens);\n        focusBorder = tokens.primary;\n        radius =")

zeus_replace_once(
    "components/button.h"
    "    core::Shadow shadow;\n    float radius"
    "    core::Shadow shadow;\n    core::Color focusBorder;\n    float radius")

zeus_replace_once(
    "components/button.h"
    "        core::Border border = style_.border;\n        border.width *= scale_;"
    "        core::Border border = style_.border;\n        border.width *= scale_;\n        const bool focused = ui_.isFocused(id_ + \".bg\");\n        if (focused) {\n            border.width = std::max(border.width, 2.0f * scale_);\n            border.color = style_.focusBorder;\n        }")

zeus_replace_once(
    "components/button.h"
    "                    .disabled(disabled_)\n                    .preserveFocusOnPress(preserveFocusOnPress_)\n                    .onClick(onClick_)"
    "                    .disabled(disabled_)\n                    .preserveFocusOnPress(preserveFocusOnPress_)\n                    .focusable()\n                    .onClick(onClick_)\n                    .onTextInput([callback = onClick_](const core::KeyboardEvent& event) {\n                        if (callback && (event.enter || event.space)) callback();\n                    })")

zeus_replace_once(
    "components/dropdown.h"
    "        const std::function<void(int)> onChange = onChange_;\n        const std::function<void(bool)> onOpenChange = onOpenChange_;"
    "        const std::function<void(int)> onChange = onChange_;\n        const std::function<void(bool)> onOpenChange = onOpenChange_;\n        core::Border fieldBorder = {metrics_.spacing.hairline, style_.border};\n        if (ui_.isFocused(id_ + \".field\")) {\n            fieldBorder.width = 2.0f;\n            fieldBorder.color = style_.accent;\n        }")

zeus_replace_if_present(
    "components/dropdown.h"
    "                        if (count <= 0 || (!event.up && !event.down && !event.home && !event.end)) {\n                            return;\n                        }\n                        int next = selected >= 0 ? selected : 0;"
    "                        const bool navigating = event.up || event.down || event.home || event.end;\n                        if (count <= 0 || !navigating) {\n                            return;\n                        }\n                        if (!open) {\n                            if (onOpenChange) onOpenChange(true);\n                            return;\n                        }\n                        int next = selected >= 0 ? selected : 0;")

zeus_replace_once(
    "components/dropdown.h"
    "                    .border(metrics_.spacing.hairline, style_.border)\n                    .transition(transition_)\n                    .onClick([onOpenChange, open = open_] {\n                        if (onOpenChange) {\n                            onOpenChange(!open);\n                        }\n                    })\n                    .build();"
    "                    .border(fieldBorder)\n                    .transition(transition_)\n                    .focusable()\n                    .onClick([onOpenChange, open = open_] {\n                        if (onOpenChange) {\n                            onOpenChange(!open);\n                        }\n                    })\n                    .onTextInput([onChange, onOpenChange, open = open_, selected, count](const core::KeyboardEvent& event) {\n                        if ((event.enter || event.space) && onOpenChange) {\n                            onOpenChange(!open);\n                            return;\n                        }\n                        if (event.escape && open && onOpenChange) {\n                            onOpenChange(false);\n                            return;\n                        }\n                        const bool navigating = event.up || event.down || event.home || event.end;\n                        if (count <= 0 || !navigating) {\n                            return;\n                        }\n                        if (!open) {\n                            if (onOpenChange) onOpenChange(true);\n                            return;\n                        }\n                        int next = selected >= 0 ? selected : 0;\n                        if (event.up) next = (next + count - 1) % count;\n                        if (event.down) next = (next + 1) % count;\n                        if (event.home) next = 0;\n                        if (event.end) next = count - 1;\n                        if (next != selected) {\n                            if (onChange) onChange(next);\n                            if (onOpenChange) onOpenChange(false);\n                        }\n                    })\n                    .build();")

zeus_replace_once(
    "components/dropdown.h"
    "                        if (next != selected && onChange) onChange(next);"
    "                        if (next != selected) {\n                            if (onChange) onChange(next);\n                            if (onOpenChange) onOpenChange(false);\n                        }")
