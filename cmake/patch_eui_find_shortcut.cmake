if(NOT DEFINED SOURCE_DIR OR NOT IS_DIRECTORY "${SOURCE_DIR}")
    message(FATAL_ERROR "SOURCE_DIR must point to an EUI-NEO checkout")
endif()

function(zeus_replace_once relative_path before after)
    set(path "${SOURCE_DIR}/${relative_path}")
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "EUI compatibility patch target is missing: ${path}")
    endif()
    file(READ "${path}" contents)
    string(FIND "${contents}" "${after}" already_patched)
    if(NOT already_patched EQUAL -1)
        return()
    endif()
    string(FIND "${contents}" "${before}" original_found)
    if(original_found EQUAL -1)
        message(FATAL_ERROR "EUI compatibility patch no longer applies to: ${path}")
    endif()
    string(REPLACE "${before}" "${after}" contents "${contents}")
    file(WRITE "${path}" "${contents}")
endfunction()

zeus_replace_once(
    "core/input/input_types.h"
    "bool escape = false;\n    bool composing = false;"
    "bool escape = false;\n    bool find = false;\n    bool composing = false;")

zeus_replace_once(
    "core/input/input_types.h"
    "undo || redo || escape;"
    "undo || redo || escape || find;")

zeus_replace_once(
    "core/input/input_types.h"
    "    Y,\n    Z\n};"
    "    Y,\n    Z,\n    F\n};")

zeus_replace_once(
    "core/input/input_state.h"
    "bool escape = false;\n    bool compositionChanged = false;"
    "bool escape = false;\n    bool find = false;\n    bool compositionChanged = false;")

zeus_replace_once(
    "core/input/input_state.h"
    "if (ctrl && key == InputKey::A) {\n        queue.selectAll = true;\n        return;\n    }"
    "if (ctrl && key == InputKey::A) {\n        queue.selectAll = true;\n        return;\n    }\n    if (ctrl && key == InputKey::F) {\n        queue.find = true;\n        return;\n    }")

zeus_replace_once(
    "core/input/input_state.h"
    "case GLFW_KEY_Z: queueKeyInput(currentWindow, InputKey::Z, ctrl, shift); break;"
    "case GLFW_KEY_Z: queueKeyInput(currentWindow, InputKey::Z, ctrl, shift); break;\n        case GLFW_KEY_F: queueKeyInput(currentWindow, InputKey::F, ctrl, shift); break;")

zeus_replace_once(
    "core/input/input_state.h"
    "keyboard.escape = queue.escape;\n    keyboard.composing = detail::isComposing(window);"
    "keyboard.escape = queue.escape;\n    keyboard.find = queue.find;\n    keyboard.composing = detail::isComposing(window);")

zeus_replace_once(
    "core/app/sdl2_app_main.cpp"
    "case SDLK_z: mapped = core::InputKey::Z; return true;"
    "case SDLK_z: mapped = core::InputKey::Z; return true;\n    case SDLK_f: mapped = core::InputKey::F; return true;")

zeus_replace_once(
    "core/dsl.h"
    "bool isFocused(const std::string& id) const {\n        return !focusedId_.empty() && focusedId_ == resolveId(id);\n    }"
    "bool isFocused(const std::string& id) const {\n        return !focusedId_.empty() && focusedId_ == resolveId(id);\n    }\n\n    void setFindShortcutTarget(const std::string& id) {\n        findShortcutTargetId_ = resolveId(id);\n    }\n\n    const std::string& findShortcutTargetId() const {\n        return findShortcutTargetId_;\n    }")

zeus_replace_once(
    "core/dsl.h"
    "std::string focusedId_;\n    StateStore stateStore_;"
    "std::string focusedId_;\n    std::string findShortcutTargetId_;\n    std::function<bool()> escapeShortcut_;\n    StateStore stateStore_;")

zeus_replace_once(
    "core/runtime/runtime_lifecycle.h"
    "if (keyboardEvent.hasInput()) {\n        updateTextInput(keyboardEvent);\n    }"
    "if (keyboardEvent.find && !ui_.findShortcutTargetId().empty()) {\n        setFocusedId(ui_.findShortcutTargetId());\n        keyboardEvent.find = false;\n    }\n    if (keyboardEvent.escape && !keyboardEvent.composing && ui_.invokeEscapeShortcut()) {\n        keyboardEvent.escape = false;\n    }\n    if (keyboardEvent.hasInput()) {\n        updateTextInput(keyboardEvent);\n    }")

zeus_replace_once(
    "core/dsl.h"
    "const std::string& findShortcutTargetId() const {\n        return findShortcutTargetId_;\n    }"
    "const std::string& findShortcutTargetId() const {\n        return findShortcutTargetId_;\n    }\n\n    void setEscapeShortcut(std::function<bool()> callback) {\n        escapeShortcut_ = std::move(callback);\n    }\n\n    bool invokeEscapeShortcut() const {\n        return escapeShortcut_ ? escapeShortcut_() : false;\n    }")
