cmake_minimum_required(VERSION 3.20)

get_filename_component(ZEUS_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(PROJECT_VERSION "0.2.0")
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 2)
set(PROJECT_VERSION_PATCH 0)
set(ZEUS_BUILD_NUMBER 2)
set(ZEUS_WINDOWS_ICON_NATIVE "${ZEUS_PROJECT_ROOT}/resources/icons/zeus-tools.ico")
set(ZEUS_TEST_RC "${ZEUS_PROJECT_ROOT}/build/windows-resource-validation.rc")
set(ZEUS_TEST_OBJ "${ZEUS_PROJECT_ROOT}/build/windows-resource-validation.o")

file(MAKE_DIRECTORY "${ZEUS_PROJECT_ROOT}/build")
configure_file("${CMAKE_CURRENT_LIST_DIR}/windows_version.rc.in" "${ZEUS_TEST_RC}" @ONLY)

find_program(ZEUS_WINDRES_EXECUTABLE NAMES x86_64-w64-mingw32-windres windres)
if(NOT ZEUS_WINDRES_EXECUTABLE)
    message(STATUS "windres is not installed; configured RC syntax only")
    return()
endif()

execute_process(
    COMMAND "${ZEUS_WINDRES_EXECUTABLE}" "${ZEUS_TEST_RC}" "${ZEUS_TEST_OBJ}"
    RESULT_VARIABLE ZEUS_WINDRES_RESULT
    OUTPUT_VARIABLE ZEUS_WINDRES_OUTPUT
    ERROR_VARIABLE ZEUS_WINDRES_ERROR
)
if(NOT ZEUS_WINDRES_RESULT EQUAL 0)
    message(FATAL_ERROR "Windows resource validation failed: ${ZEUS_WINDRES_ERROR}")
endif()
message(STATUS "Windows icon and VersionInfo resource compiled successfully")
