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
    "enum class CursorShape {\n    Arrow,\n    Hand\n};"
    "enum class CursorShape {\n    Arrow,\n    Hand,\n    ResizeVertical\n};")

zeus_replace_once(
    "core/window/window_types.h"
    "enum class CursorType {\n    Arrow,\n    Hand\n};"
    "enum class CursorType {\n    Arrow,\n    Hand,\n    ResizeVertical\n};")

zeus_replace_once(
    "core/window/window_backend.cpp"
    "return SDL_CreateSystemCursor(type == CursorType::Hand ? SDL_SYSTEM_CURSOR_HAND : SDL_SYSTEM_CURSOR_ARROW);"
    "const SDL_SystemCursor cursor = type == CursorType::Hand\n        ? SDL_SYSTEM_CURSOR_HAND\n        : (type == CursorType::ResizeVertical ? SDL_SYSTEM_CURSOR_SIZENS : SDL_SYSTEM_CURSOR_ARROW);\n    return SDL_CreateSystemCursor(cursor);")

zeus_replace_once(
    "core/window/window_backend.cpp"
    "return glfwCreateStandardCursor(type == CursorType::Hand ? GLFW_HAND_CURSOR : GLFW_ARROW_CURSOR);"
    "const int cursor = type == CursorType::Hand\n        ? GLFW_HAND_CURSOR\n        : (type == CursorType::ResizeVertical ? GLFW_VRESIZE_CURSOR : GLFW_ARROW_CURSOR);\n    return glfwCreateStandardCursor(cursor);")

zeus_replace_once(
    "core/dsl_runtime.h"
    "bool wantsHandCursor_ = false;"
    "bool wantsHandCursor_ = false;\n    bool wantsVerticalResizeCursor_ = false;")

zeus_replace_once(
    "core/dsl_runtime.h"
    "core::window::CursorHandle handCursor_ = nullptr;"
    "core::window::CursorHandle handCursor_ = nullptr;\n    core::window::CursorHandle verticalResizeCursor_ = nullptr;")

zeus_replace_once(
    "core/runtime/runtime_input.h"
    "if (enabled && instance.state.hover && element.cursor == CursorShape::Hand) {\n        wantsHandCursor_ = true;\n    }"
    "if (enabled && instance.state.hover && element.cursor == CursorShape::Hand) {\n        wantsHandCursor_ = true;\n    }\n    if (enabled && instance.state.hover && element.cursor == CursorShape::ResizeVertical) {\n        wantsVerticalResizeCursor_ = true;\n    }")

zeus_replace_once(
    "core/runtime/runtime_lifecycle.h"
    "wantsHandCursor_ = false;"
    "wantsHandCursor_ = false;\n    wantsVerticalResizeCursor_ = false;")

zeus_replace_once(
    "core/runtime/runtime_lifecycle.h"
    "if (!handCursor_) {\n        handCursor_ = core::window::createStandardCursor(core::window::CursorType::Hand);\n    }\n\n    core::window::CursorHandle target = wantsHandCursor_ && handCursor_ ? handCursor_ : arrowCursor_;"
    "if (!handCursor_) {\n        handCursor_ = core::window::createStandardCursor(core::window::CursorType::Hand);\n    }\n    if (!verticalResizeCursor_) {\n        verticalResizeCursor_ = core::window::createStandardCursor(core::window::CursorType::ResizeVertical);\n    }\n\n    core::window::CursorHandle target = wantsVerticalResizeCursor_ && verticalResizeCursor_\n        ? verticalResizeCursor_\n        : (wantsHandCursor_ && handCursor_ ? handCursor_ : arrowCursor_);")

zeus_replace_once(
    "core/runtime/runtime_lifecycle.h"
    "if (handCursor_) {\n        core::window::destroyCursor(handCursor_);\n        handCursor_ = nullptr;\n    }"
    "if (handCursor_) {\n        core::window::destroyCursor(handCursor_);\n        handCursor_ = nullptr;\n    }\n    if (verticalResizeCursor_) {\n        core::window::destroyCursor(verticalResizeCursor_);\n        verticalResizeCursor_ = nullptr;\n    }")
