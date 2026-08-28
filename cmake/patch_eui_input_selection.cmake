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

function(zeus_replace_if_present relative_path before after)
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
        return()
    endif()
    string(REPLACE "${before}" "${after}" contents "${contents}")
    file(WRITE "${path}" "${contents}")
endfunction()

zeus_replace_once(
    "components/input.h"
    "    InputBuilder& onFocus(std::function<void(bool)> callback) {\n        onFocus_ = std::move(callback);\n        return *this;\n    }"
    "    InputBuilder& onFocus(std::function<void(bool)> callback) {\n        onFocus_ = std::move(callback);\n        return *this;\n    }\n    InputBuilder& onSelectionStart(std::function<void(int, int)> callback) {\n        onSelectionStart_ = std::move(callback);\n        return *this;\n    }\n    InputBuilder& onSelectionChange(std::function<void(int, int)> callback) {\n        onSelectionChange_ = std::move(callback);\n        return *this;\n    }\n    InputBuilder& onSelectionEnd(std::function<void(int, int)> callback) {\n        onSelectionEnd_ = std::move(callback);\n        return *this;\n    }\n    InputBuilder& onCopy(std::function<void()> callback) {\n        onCopy_ = std::move(callback);\n        return *this;\n    }")

zeus_replace_once(
    "components/input.h"
    "        const std::function<void(bool)> onFocus = onFocus_;"
    "        const std::function<void(bool)> onFocus = onFocus_;\n        const std::function<void(int, int)> onSelectionStart = onSelectionStart_;\n        const std::function<void(int, int)> onSelectionChange = onSelectionChange_;\n        const std::function<void(int, int)> onSelectionEnd = onSelectionEnd_;\n        const std::function<void()> onCopy = onCopy_;")

zeus_replace_once(
    "components/input.h"
    "    InputBuilder& onCopy(std::function<void()> callback) {\n        onCopy_ = std::move(callback);\n        return *this;\n    }"
    "    InputBuilder& onCopy(std::function<void()> callback) {\n        onCopy_ = std::move(callback);\n        return *this;\n    }\n    InputBuilder& onSelectAll(std::function<void()> callback) {\n        onSelectAll_ = std::move(callback);\n        return *this;\n    }")

zeus_replace_once(
    "components/input.h"
    "        const std::function<void()> onCopy = onCopy_;"
    "        const std::function<void()> onCopy = onCopy_;\n        const std::function<void()> onSelectAll = onSelectAll_;")

zeus_replace_once(
    "components/input.h"
    ".onPress([&state, width, inset, layout](const core::PointerEvent& event, const core::Rect& bounds) {"
    ".onPress([&state, width, inset, layout, onSelectionStart, onSelectionChange](const core::PointerEvent& event, const core::Rect& bounds) {")

zeus_replace_once(
    "components/input.h"
    "                        state.selecting = true;\n                    })\n                    .onFocusChanged(onFocus)"
    "                        state.selecting = true;\n                        if (onSelectionStart) onSelectionStart(state.selectionStart, state.selectionEnd);\n                        if (onSelectionChange) onSelectionChange(state.selectionStart, state.selectionEnd);\n                    })\n                    .onRelease([&state, onSelectionEnd](const core::PointerEvent&, const core::Rect&) {\n                        state.selecting = false;\n                        if (onSelectionEnd) onSelectionEnd(state.selectionStart, state.selectionEnd);\n                    })\n                    .onFocusChanged(onFocus)")

zeus_replace_once(
    "components/input.h"
    ".onDrag([&state, width, inset, fontSize, fontFamily, allowMultiline, textHeight, layout](const core::dsl::DragEvent& event) {"
    ".onDrag([&state, width, inset, fontSize, fontFamily, allowMultiline, textHeight, layout, onSelectionChange](const core::dsl::DragEvent& event) {")

zeus_replace_once(
    "components/input.h"
    "                            InputModel::syncScroll(state, std::max(0.0f, width - inset * 2.0f), fontFamily, fontSize);\n                        }\n                    });"
    "                            InputModel::syncScroll(state, std::max(0.0f, width - inset * 2.0f), fontFamily, fontSize);\n                        }\n                        if (onSelectionChange) onSelectionChange(state.selectionStart, state.selectionEnd);\n                    });")

zeus_replace_if_present(
    "components/input.h"
    "hit.onTextInput([&state, allowMultiline, onChange, onEnter, onSelectionChange, onCopy, onSelectAll, width, inset, fontSize, fontFamily, textHeight](const core::KeyboardEvent& event) {"
    "hit.onTextInput([&state, allowMultiline, onChange, onEnter, onSelectionChange, onCopy, onSelectAll, onHistory, width, inset, fontSize, fontFamily, textHeight](const core::KeyboardEvent& event) {")

zeus_replace_once(
    "components/input.h"
    "hit.onTextInput([&state, allowMultiline, onChange, onEnter, width, inset, fontSize, fontFamily, textHeight](const core::KeyboardEvent& event) {"
    "hit.onTextInput([&state, allowMultiline, onChange, onEnter, onSelectionChange, onCopy, onSelectAll, onHistory, width, inset, fontSize, fontFamily, textHeight](const core::KeyboardEvent& event) {")

zeus_replace_once(
    "components/input.h"
    "                        if (event.selectAll) {\n                            state.selectionStart = 0;\n                            state.selectionEnd = static_cast<int>(state.text.size());\n                            state.cursor = state.selectionEnd;\n                        }"
    "                        if (event.selectAll) {\n                            state.selectionStart = 0;\n                            state.selectionEnd = static_cast<int>(state.text.size());\n                            state.cursor = state.selectionEnd;\n                            if (onSelectAll) onSelectAll();\n                        }")

zeus_replace_once(
    "components/input.h"
    "                            if (changed && onChange) {\n                                onChange(state.text);\n                            }\n                            return;"
    "                            if (changed && onChange) {\n                                onChange(state.text);\n                            }\n                            if (onSelectionChange) onSelectionChange(state.selectionStart, state.selectionEnd);\n                            return;")

zeus_replace_once(
    "components/input.h"
    "                        if (event.copy) {\n                            InputModel::copySelection(state);\n                        }"
    "                        if (event.copy) {\n                            if (onCopy) onCopy();\n                            else InputModel::copySelection(state);\n                        }")

zeus_replace_once(
    "components/input.h"
    "                            InputModel::copySelection(state);\n                            InputModel::pushUndoState(state);"
    "                            if (onCopy) onCopy();\n                            else InputModel::copySelection(state);\n                            InputModel::pushUndoState(state);")

zeus_replace_once(
    "components/input.h"
    "                        if (changed && onChange) {\n                            onChange(state.text);\n                        }\n                    })"
    "                        if (changed && onChange) {\n                            onChange(state.text);\n                        }\n                        if (onSelectionChange) onSelectionChange(state.selectionStart, state.selectionEnd);\n                    })")

zeus_replace_once(
    "components/input.h"
    "    std::function<void(bool)> onFocus_;"
    "    std::function<void(bool)> onFocus_;\n    std::function<void(int, int)> onSelectionStart_;\n    std::function<void(int, int)> onSelectionChange_;\n    std::function<void(int, int)> onSelectionEnd_;\n    std::function<void()> onCopy_;")

zeus_replace_once(
    "components/input.h"
    "    std::function<void()> onCopy_;"
    "    std::function<void()> onCopy_;\n    std::function<void()> onSelectAll_;")

zeus_replace_once(
    "components/input.h"
    "    InputBuilder& onSelectAll(std::function<void()> callback) {\n        onSelectAll_ = std::move(callback);\n        return *this;\n    }"
    "    InputBuilder& onSelectAll(std::function<void()> callback) {\n        onSelectAll_ = std::move(callback);\n        return *this;\n    }\n    InputBuilder& onHistory(std::function<bool(bool)> callback) {\n        onHistory_ = std::move(callback);\n        return *this;\n    }")

zeus_replace_once(
    "components/input.h"
    "        const std::function<void()> onSelectAll = onSelectAll_;"
    "        const std::function<void()> onSelectAll = onSelectAll_;\n        const std::function<bool(bool)> onHistory = onHistory_;")

zeus_replace_once(
    "components/input.h"
    "hit.onTextInput([&state, allowMultiline, onChange, onEnter, onSelectionChange, onCopy, onSelectAll, width, inset, fontSize, fontFamily, textHeight](const core::KeyboardEvent& event) {"
    "hit.onTextInput([&state, allowMultiline, onChange, onEnter, onSelectionChange, onCopy, onSelectAll, onHistory, width, inset, fontSize, fontFamily, textHeight](const core::KeyboardEvent& event) {")

zeus_replace_once(
    "components/input.h"
    "                        if (event.undo || event.redo) {"
    "                        if ((event.undo || event.redo) && onHistory && onHistory(event.redo)) {\n                            return;\n                        }\n                        if (event.undo || event.redo) {")

zeus_replace_once(
    "components/input.h"
    "    std::function<void()> onSelectAll_;"
    "    std::function<void()> onSelectAll_;\n    std::function<bool(bool)> onHistory_;")
