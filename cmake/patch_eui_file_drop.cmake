if(NOT DEFINED SOURCE_DIR OR NOT IS_DIRECTORY "${SOURCE_DIR}")
    message(FATAL_ERROR "SOURCE_DIR must point to an EUI-NEO checkout")
endif()

function(zeus_replace_once relative_path before after)
    set(path "${SOURCE_DIR}/${relative_path}")
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "EUI file-drop patch target is missing: ${path}")
    endif()
    file(READ "${path}" contents)
    string(FIND "${contents}" "${after}" already_patched)
    if(NOT already_patched EQUAL -1)
        return()
    endif()
    string(FIND "${contents}" "${before}" original_found)
    if(original_found EQUAL -1)
        message(FATAL_ERROR "EUI file-drop patch no longer applies to: ${path}")
    endif()
    string(REPLACE "${before}" "${after}" contents "${contents}")
    file(WRITE "${path}" "${contents}")
endfunction()

zeus_replace_once(
    "core/app/glfw_app_main.cpp"
    "#include <memory>\n#include <thread>\n#include <vector>"
    "#include <memory>\n#include <string>\n#include <thread>\n#include <vector>\n\nnamespace app {\nvoid onFilesDropped(const std::vector<std::string>& paths);\n}")

zeus_replace_once(
    "core/app/glfw_app_main.cpp"
    "    glfwSetWindowIconifyCallback(window, [](GLFWwindow* currentWindow, int iconified) {\n        WindowState* state = static_cast<WindowState*>(glfwGetWindowUserPointer(currentWindow));\n        if (!state) {\n            return;\n        }\n        state->iconified = iconified == GLFW_TRUE;\n        if (!state->iconified) {\n            state->paintRequested = true;\n        }\n    });\n}"
    "    glfwSetWindowIconifyCallback(window, [](GLFWwindow* currentWindow, int iconified) {\n        WindowState* state = static_cast<WindowState*>(glfwGetWindowUserPointer(currentWindow));\n        if (!state) {\n            return;\n        }\n        state->iconified = iconified == GLFW_TRUE;\n        if (!state->iconified) {\n            state->paintRequested = true;\n        }\n    });\n    glfwSetDropCallback(window, [](GLFWwindow* currentWindow, int count, const char** paths) {\n        std::vector<std::string> dropped;\n        dropped.reserve(static_cast<std::size_t>(std::max(count, 0)));\n        for (int index = 0; index < count; ++index) {\n            if (paths[index] != nullptr) dropped.emplace_back(paths[index]);\n        }\n        if (!dropped.empty()) app::onFilesDropped(dropped);\n        WindowState* state = static_cast<WindowState*>(glfwGetWindowUserPointer(currentWindow));\n        if (state != nullptr) state->paintRequested = true;\n        app::detail::requestFullPaint();\n    });\n}")
