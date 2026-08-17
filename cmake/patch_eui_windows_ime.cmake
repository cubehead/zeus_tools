if(NOT DEFINED SOURCE_DIR OR NOT IS_DIRECTORY "${SOURCE_DIR}")
    message(FATAL_ERROR "SOURCE_DIR must point to an EUI-NEO checkout")
endif()

set(path "${SOURCE_DIR}/core/input/input_state.h")
if(NOT EXISTS "${path}")
    message(FATAL_ERROR "EUI Windows IME patch target is missing: ${path}")
endif()

file(READ "${path}" contents)
set(before
"glfwSetCharCallback(glfwWindow, [](GLFWwindow* currentWindow, unsigned int codepoint) {
        detail::InputQueue& queue = detail::inputQueue(currentWindow);")
set(after
"glfwSetCharCallback(glfwWindow, [](GLFWwindow* currentWindow, unsigned int codepoint) {
#if defined(_WIN32)
        // Windows can emit the current pinyin keystroke through GLFW's char
        // callback while the IME still owns an active composition. Treating
        // that character as committed text cancels the composition after each
        // key and produces output such as \"zhongwe你\" instead of \"中文\".
        // The selected candidate arrives after the composition ends and must
        // continue through the normal committed-text path below.
        if (eui_ime_is_composing(currentWindow) != 0) {
            return;
        }
#endif
        detail::InputQueue& queue = detail::inputQueue(currentWindow);")

string(FIND "${contents}" "${after}" already_patched)
if(NOT already_patched EQUAL -1)
    return()
endif()

string(FIND "${contents}" "${before}" original_found)
if(original_found EQUAL -1)
    message(FATAL_ERROR "EUI Windows IME patch no longer applies to: ${path}")
endif()

string(REPLACE "${before}" "${after}" contents "${contents}")
file(WRITE "${path}" "${contents}")
