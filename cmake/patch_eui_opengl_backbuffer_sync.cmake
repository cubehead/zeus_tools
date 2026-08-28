if(NOT DEFINED SOURCE_DIR OR NOT IS_DIRECTORY "${SOURCE_DIR}")
    message(FATAL_ERROR "SOURCE_DIR must point to an EUI-NEO checkout")
endif()

set(path "${SOURCE_DIR}/core/render/opengl/opengl_backend.cpp")
if(NOT EXISTS "${path}")
    message(FATAL_ERROR "EUI OpenGL backbuffer patch target is missing: ${path}")
endif()

file(READ "${path}" contents)

set(before [=[
bool platformRequiresConservativeBackbufferSync() {
#if defined(__linux__) || defined(_WIN32)
    // The backbuffer contents and swap order are not guaranteed after presenting.
    // Use full cache blits instead of assuming a preserved two-buffer swap chain.
    return true;
#else
    return false;
#endif
}
]=])

set(after [=[
bool platformRequiresConservativeBackbufferSync() {
    // OpenGL does not guarantee that a default framebuffer keeps its contents
    // after a swap, nor that a compositor exposes exactly two buffers. Always
    // synchronize from the retained cache before presenting an existing frame.
    // Without this, dismissed overlays can reappear from a stale macOS buffer.
    return true;
}
]=])

string(FIND "${contents}" "${after}" already_patched)
if(NOT already_patched EQUAL -1)
    return()
endif()

string(FIND "${contents}" "${before}" original_found)
if(original_found EQUAL -1)
    message(FATAL_ERROR "EUI OpenGL backbuffer patch no longer applies to: ${path}")
endif()

string(REPLACE "${before}" "${after}" contents "${contents}")
file(WRITE "${path}" "${contents}")
