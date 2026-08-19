cmake_minimum_required(VERSION 3.20)

project(ZeusTools VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # Source, fixtures and localized UI strings are UTF-8. MSVC otherwise
    # interprets them using the active Windows code page (for example CP936).
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:/utf-8>
        $<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
endif()

option(ZEUS_BUILD_APP "Build the EUI-NEO desktop prototype" ON)
option(ZEUS_BUILD_CLI "Build the Zeus Tools command-line pipeline" ON)
option(ZEUS_BUILD_TESTS "Build Zeus core tests" ON)
option(ZEUS_ENABLE_PACKAGING "Configure native application bundles and CPack packages" OFF)
option(ZEUS_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
set(ZEUS_EUI_NEO_SOURCE_DIR "" CACHE PATH "Path to an existing EUI-NEO v0.5.3 checkout")
set(ZEUS_APP_IDENTIFIER "io.github.zeustools.app" CACHE STRING "macOS application bundle identifier")
set(ZEUS_BUILD_NUMBER "2" CACHE STRING "Numeric platform build number")
set(ZEUS_MACOS_MIN_VERSION "12.0" CACHE STRING "Minimum supported macOS version")
set(ZEUS_CODESIGN_IDENTITY "" CACHE STRING "Optional macOS Developer ID Application identity")
set(ZEUS_DOCS_SCENARIO "" CACHE STRING "Optional synthetic README screenshot scenario")

if(ZEUS_ENABLE_SANITIZERS)
    if(MSVC)
        message(FATAL_ERROR "ZEUS_ENABLE_SANITIZERS currently requires Clang or GCC")
    endif()
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:-fsanitize=address,undefined>
        $<$<COMPILE_LANGUAGE:C>:-fno-omit-frame-pointer>
        $<$<COMPILE_LANGUAGE:C>:-fno-sanitize-recover=all>
        $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=address,undefined>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-omit-frame-pointer>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-sanitize-recover=all>)
    add_link_options(-fsanitize=address,undefined -fno-sanitize-recover=all)
endif()

if(APPLE)
    # Keep the Mach-O load command and Info.plist minimum version in sync. A
    # plist-only declaration does not make a binary compatible with older macOS.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "${ZEUS_MACOS_MIN_VERSION}"
        CACHE STRING "Minimum macOS deployment target" FORCE)
endif()

include(FetchContent)
set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PUGIXML_BUILD_SHARED_AND_STATIC_LIBS OFF CACHE BOOL "" FORCE)
set(PUGIXML_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    # v1.15; pin the immutable commit rather than the movable tag.
    GIT_TAG ee86beb30e4973f5feffe3ce63bfa4fbadf72f38
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(pugixml)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    # yaml-cpp-0.9.0; pin the immutable commit rather than the movable tag.
    GIT_TAG 56e3bb550c91fd7005566f19c079cb7a503223cf
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(yaml-cpp)

set(TOMLPLUSPLUS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TOMLPLUSPLUS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    # v3.4.0; pin the immutable commit rather than the movable tag.
    GIT_TAG 30172438cee64926dc41fdd9c11fb3ba5b2ba9de
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(tomlplusplus)

if(APPLE AND ZEUS_BUILD_APP)
    enable_language(OBJC OBJCXX)
endif()

add_library(zeus_core
    src/core/crypto_service.cpp
    src/core/csv_document.cpp
    src/core/developer_tools.cpp
    src/core/ini_formatter.cpp
    src/core/json_formatter.cpp
    src/core/locale_preference.cpp
    src/core/secure_memory.cpp
    src/core/structured_converter.cpp
    src/core/system_ui_font.cpp
    src/core/text_selection.cpp
    src/core/text_document.cpp
    src/core/text_processor.cpp
    src/core/theme_preference.cpp
    src/core/toml_formatter.cpp
    src/core/xml_formatter.cpp
    src/core/yaml_formatter.cpp
)
target_include_directories(zeus_core PUBLIC include)
target_compile_features(zeus_core PUBLIC cxx_std_17)
target_link_libraries(zeus_core PUBLIC
    pugixml::pugixml yaml-cpp::yaml-cpp tomlplusplus::tomlplusplus)

add_library(zeus_app_processing
    src/app/processing_registry.cpp
    src/app/processing_service.cpp
)
target_include_directories(zeus_app_processing PUBLIC src/app)
target_link_libraries(zeus_app_processing PUBLIC zeus_core)
target_compile_features(zeus_app_processing PUBLIC cxx_std_17)

if(ZEUS_BUILD_CLI)
    add_executable(zeus_cli src/cli/main.cpp)
    set_target_properties(zeus_cli PROPERTIES OUTPUT_NAME "zeus-tools-cli")
    target_link_libraries(zeus_cli PRIVATE zeus_app_processing)
    target_compile_features(zeus_cli PRIVATE cxx_std_17)
    target_compile_definitions(zeus_cli PRIVATE ZEUS_CLI_VERSION="${PROJECT_VERSION}")
    if(MINGW)
        # The Windows artifact is a portable ZIP. Do not require users to
        # install or locate the MinGW C++, GCC or pthread runtime DLLs.
        target_link_options(zeus_cli PRIVATE -static)
    endif()
endif()

if(WIN32)
    target_link_libraries(zeus_core PUBLIC bcrypt)
endif()

if(MSVC)
    target_compile_options(zeus_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(zeus_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ZEUS_BUILD_APP)
    set(EUI_BUILD_APPS OFF CACHE BOOL "" FORCE)
    set(EUI_BUILD_USER_APPS OFF CACHE BOOL "" FORCE)
    set(EUI_BUILD_TEST_FIXTURES OFF CACHE BOOL "" FORCE)
    set(EUI_ENABLE_MODULES OFF CACHE BOOL "" FORCE)
    set(EUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

    if(ZEUS_EUI_NEO_SOURCE_DIR)
        add_subdirectory("${ZEUS_EUI_NEO_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/eui-neo")
        set(ZEUS_EUI_APP_MAIN "${ZEUS_EUI_NEO_SOURCE_DIR}/core/app/glfw_app_main.cpp")
        set(ZEUS_EUI_SOURCE_ROOT "${ZEUS_EUI_NEO_SOURCE_DIR}")
    else()
        FetchContent_Declare(
            eui_neo
            GIT_REPOSITORY https://github.com/sudoevolve/EUI-NEO.git
            GIT_TAG f2a3b72104bd946988f8ebe0a13dda956f3455ae
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(eui_neo)
        set(ZEUS_EUI_APP_MAIN "${eui_neo_SOURCE_DIR}/core/app/glfw_app_main.cpp")
        set(ZEUS_EUI_SOURCE_ROOT "${eui_neo_SOURCE_DIR}")
    endif()

    # Apply small, idempotent EUI-NEO v0.5.3 compatibility patches for the
    # native splitter cursor, desktop shortcuts and file-drop input.
    execute_process(
        COMMAND ${CMAKE_COMMAND}
                -DSOURCE_DIR=${ZEUS_EUI_SOURCE_ROOT}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_eui_vertical_resize_cursor.cmake
        RESULT_VARIABLE ZEUS_EUI_CURSOR_PATCH_RESULT
    )
    if(NOT ZEUS_EUI_CURSOR_PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to add vertical-resize cursor support to EUI-NEO")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND}
                -DSOURCE_DIR=${ZEUS_EUI_SOURCE_ROOT}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_eui_find_shortcut.cmake
        RESULT_VARIABLE ZEUS_EUI_FIND_PATCH_RESULT
    )
    if(NOT ZEUS_EUI_FIND_PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to add find-shortcut support to EUI-NEO")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND}
                -DSOURCE_DIR=${ZEUS_EUI_SOURCE_ROOT}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_eui_file_drop.cmake
        RESULT_VARIABLE ZEUS_EUI_FILE_DROP_PATCH_RESULT
    )
    if(NOT ZEUS_EUI_FILE_DROP_PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to add file-drop support to EUI-NEO")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND}
                -DSOURCE_DIR=${ZEUS_EUI_SOURCE_ROOT}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_eui_windows_ime.cmake
        RESULT_VARIABLE ZEUS_EUI_WINDOWS_IME_PATCH_RESULT
    )
    if(NOT ZEUS_EUI_WINDOWS_IME_PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to preserve Windows IME compositions in EUI-NEO")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND}
                -DSOURCE_DIR=${ZEUS_EUI_SOURCE_ROOT}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_eui_data_image.cmake
        RESULT_VARIABLE ZEUS_EUI_DATA_IMAGE_PATCH_RESULT
    )
    if(NOT ZEUS_EUI_DATA_IMAGE_PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to add in-memory Data URL image support to EUI-NEO")
    endif()

    set(ZEUS_APP_SOURCES
        "${ZEUS_EUI_APP_MAIN}"
        src/app/app.cpp
        src/app/app_controller.cpp
        src/app/i18n.cpp
        src/app/main_view.cpp
        src/app/views/about_dialog.cpp
        src/app/views/action_bar.cpp
        src/app/views/crypto_panel.cpp
        src/app/views/header_bar.cpp
        src/app/views/input_panel.cpp
        src/app/views/large_input_editor.cpp
        src/app/views/result_panel.cpp
        src/app/views/search_bar.cpp
    )
    if(APPLE)
        list(APPEND ZEUS_APP_SOURCES
            src/platform/file_dialog_macos.mm
            src/platform/system_theme_macos.m
            src/platform/system_ui_font_macos.m
        )
    elseif(WIN32)
        list(APPEND ZEUS_APP_SOURCES
            src/platform/file_dialog_windows.cpp
            src/platform/system_theme_windows.cpp
            src/platform/system_ui_font_windows.cpp
        )
    else()
        list(APPEND ZEUS_APP_SOURCES
            src/platform/file_dialog_fallback.cpp
            src/platform/system_theme_fallback.cpp
            src/platform/system_ui_font_fallback.cpp
        )
    endif()
    if(APPLE AND ZEUS_ENABLE_PACKAGING)
        set(ZEUS_MACOS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/zeus-tools.icns")
        set(ZEUS_BUNDLE_EXECUTABLE "Zeus Tools")
        set_source_files_properties("${ZEUS_MACOS_ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        configure_file(cmake/Info.plist.in "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" @ONLY)
        add_executable(zeus_tools MACOSX_BUNDLE ${ZEUS_APP_SOURCES} "${ZEUS_MACOS_ICON}")
        set_target_properties(zeus_tools PROPERTIES
            OUTPUT_NAME "${ZEUS_BUNDLE_EXECUTABLE}"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
            MACOSX_BUNDLE_BUNDLE_NAME "Zeus Tools"
            MACOSX_BUNDLE_GUI_IDENTIFIER "${ZEUS_APP_IDENTIFIER}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${ZEUS_BUILD_NUMBER}"
        )
    elseif(WIN32 AND ZEUS_ENABLE_PACKAGING)
        file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/zeus-tools.ico" ZEUS_WINDOWS_ICON_NATIVE)
        string(REPLACE "\\" "\\\\" ZEUS_WINDOWS_ICON_NATIVE "${ZEUS_WINDOWS_ICON_NATIVE}")
        configure_file(cmake/windows_version.rc.in "${CMAKE_CURRENT_BINARY_DIR}/zeus_tools.rc" @ONLY)
        add_executable(zeus_tools WIN32 ${ZEUS_APP_SOURCES} "${CMAKE_CURRENT_BINARY_DIR}/zeus_tools.rc")
        set_target_properties(zeus_tools PROPERTIES OUTPUT_NAME "ZeusTools")
    else()
        add_executable(zeus_tools ${ZEUS_APP_SOURCES})
    endif()
    target_link_libraries(zeus_tools PRIVATE zeus_app_processing)
    if(MINGW)
        target_link_options(zeus_tools PRIVATE -static)
    endif()
    if(WIN32)
        target_link_libraries(zeus_tools PRIVATE Advapi32 Comdlg32)
    elseif(APPLE)
        target_link_libraries(zeus_tools PRIVATE
            "-framework AppKit" "-framework CoreText" "-framework CoreFoundation")
    endif()
    eui_neo_configure_app(zeus_tools)
    target_compile_definitions(zeus_tools PRIVATE
        ZEUS_APP_VERSION="${PROJECT_VERSION}"
        ZEUS_APP_BUILD_NUMBER="${ZEUS_BUILD_NUMBER}")
    if(NOT ZEUS_DOCS_SCENARIO STREQUAL "")
        target_compile_definitions(zeus_tools PRIVATE
            ZEUS_DOCS_SCENARIO="${ZEUS_DOCS_SCENARIO}")
    endif()
    # EUI itself remains exception-free in release builds, but Zeus performs
    # parsing in application-owned worker callbacks. Keep exceptions enabled
    # for this target so those callbacks can turn failures into UI errors.
    if(MSVC)
        target_compile_options(zeus_tools PRIVATE /EHsc)
    else()
        target_compile_options(zeus_tools PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>)
    endif()
    # Copy only the runtime asset Zeus Tools actually uses. Avoid shipping the
    # complete EUI gallery asset directory and unrelated demo fonts.
    add_custom_command(TARGET zeus_tools POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:zeus_tools>/assets"
        # EUI-NEO used to copy its complete demo asset directory. Remove those
        # known stale files so an incremental release build cannot package them.
        COMMAND ${CMAKE_COMMAND} -E rm -f
                "$<TARGET_FILE_DIR:zeus_tools>/assets/JingNanJunJunTi-JinNanJunJunTi-Bold-2.ttf"
                "$<TARGET_FILE_DIR:zeus_tools>/assets/YouSheBiaoTiHei-2.ttf"
                "$<TARGET_FILE_DIR:zeus_tools>/assets/icon.ico"
                "$<TARGET_FILE_DIR:zeus_tools>/assets/icon.png"
                "$<TARGET_FILE_DIR:zeus_tools>/assets/icon.svg"
                "$<TARGET_FILE_DIR:zeus_tools>/assets/mona-loading-default.gif"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ZEUS_EUI_SOURCE_ROOT}/assets/Font Awesome 7 Free-Solid-900.otf"
                "$<TARGET_FILE_DIR:zeus_tools>/assets/Font Awesome 7 Free-Solid-900.otf"
        VERBATIM
        COMMENT "Copying EUI runtime assets"
    )
    add_custom_target(zeus_runtime_icon ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:zeus_tools>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/zeus-tools-1024.png"
                "$<TARGET_FILE_DIR:zeus_tools>/assets/zeus-tools-1024.png"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/github-mark.svg"
                "$<TARGET_FILE_DIR:zeus_tools>/assets/github-mark.svg"
        DEPENDS
                "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/zeus-tools-1024.png"
                "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/github-mark.svg"
        VERBATIM
        COMMENT "Updating Zeus Tools runtime icon"
    )
    add_dependencies(zeus_tools zeus_runtime_icon)

    if(APPLE AND ZEUS_ENABLE_PACKAGING AND ZEUS_CODESIGN_IDENTITY)
        find_program(ZEUS_CODESIGN_EXECUTABLE codesign REQUIRED)
        add_custom_command(TARGET zeus_tools POST_BUILD
            COMMAND "${ZEUS_CODESIGN_EXECUTABLE}" --force --deep --options runtime --timestamp
                    --sign "${ZEUS_CODESIGN_IDENTITY}" "$<TARGET_BUNDLE_DIR:zeus_tools>"
            VERBATIM
            COMMENT "Signing Zeus Tools.app"
        )
    endif()
endif()

if(ZEUS_BUILD_TESTS)
    enable_testing()
    add_library(zeus_test_support tests/detection_corpus.cpp)
    target_include_directories(zeus_test_support PUBLIC tests)

    add_executable(zeus_core_tests tests/core_tests.cpp)
    target_link_libraries(zeus_core_tests PRIVATE zeus_core zeus_test_support)
    target_compile_definitions(zeus_core_tests PRIVATE
        ZEUS_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    add_test(NAME zeus_core_tests COMMAND zeus_core_tests)

    add_executable(zeus_detection_report tests/detection_report.cpp)
    target_link_libraries(zeus_detection_report PRIVATE zeus_core zeus_test_support)
    target_compile_definitions(zeus_detection_report PRIVATE
        ZEUS_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    add_test(NAME zeus_detection_quality COMMAND zeus_detection_report)

    add_executable(zeus_app_processing_tests tests/app_processing_tests.cpp)
    target_link_libraries(zeus_app_processing_tests PRIVATE zeus_app_processing)
    add_test(NAME zeus_app_processing_tests COMMAND zeus_app_processing_tests)

    if(ZEUS_BUILD_APP)
        add_executable(zeus_data_image_tests tests/data_image_tests.cpp)
        target_link_libraries(zeus_data_image_tests PRIVATE eui_neo)
        add_test(NAME zeus_data_image_tests COMMAND zeus_data_image_tests)
    endif()

    if(ZEUS_BUILD_CLI)
        add_test(NAME zeus_cli_smoke
            COMMAND ${CMAKE_COMMAND}
                -DZEUS_CLI=$<TARGET_FILE:zeus_cli>
                -DZEUS_FIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_smoke.cmake)
    endif()

    add_executable(zeus_core_benchmark tests/core_benchmark.cpp)
    target_link_libraries(zeus_core_benchmark PRIVATE zeus_core)

    add_executable(zeus_csv_benchmark tests/csv_benchmark.cpp)
    target_link_libraries(zeus_csv_benchmark PRIVATE zeus_core)

    add_executable(zeus_app_processing_benchmark tests/app_processing_benchmark.cpp)
    target_link_libraries(zeus_app_processing_benchmark PRIVATE zeus_app_processing)
endif()

if(ZEUS_ENABLE_PACKAGING)
    if(NOT ZEUS_BUILD_APP)
        message(FATAL_ERROR "ZEUS_ENABLE_PACKAGING requires ZEUS_BUILD_APP=ON")
    endif()

    install(TARGETS zeus_tools
        BUNDLE DESTINATION .
        RUNTIME DESTINATION .
    )
    if(ZEUS_BUILD_CLI)
        install(TARGETS zeus_cli RUNTIME DESTINATION .)
    endif()
    if(WIN32)
        install(FILES "${ZEUS_EUI_SOURCE_ROOT}/assets/Font Awesome 7 Free-Solid-900.otf"
                DESTINATION assets)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/zeus-tools-1024.png"
                DESTINATION assets)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/github-mark.svg"
                DESTINATION assets)
    endif()
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        "${CMAKE_CURRENT_SOURCE_DIR}/PRIVACY.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
        DESTINATION .)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/licenses/"
        DESTINATION licenses)

    set(CPACK_PACKAGE_NAME "Zeus Tools")
    set(CPACK_PACKAGE_VENDOR "Zeus Tools contributors")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Offline formatter, decoder and developer toolbox")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
    set(CPACK_PACKAGE_CHECKSUM SHA256)
    set(CPACK_STRIP_FILES TRUE)

    if(APPLE)
        set(CPACK_GENERATOR "DragNDrop")
        set(CPACK_DMG_VOLUME_NAME "Zeus Tools ${PROJECT_VERSION}")
        set(CPACK_PACKAGE_FILE_NAME "Zeus-Tools-${PROJECT_VERSION}-macOS-${CMAKE_SYSTEM_PROCESSOR}")
    elseif(WIN32)
        # Windows ships as a portable ZIP by product decision; no installer generator.
        set(CPACK_GENERATOR "ZIP")
        set(CPACK_PACKAGE_FILE_NAME "Zeus-Tools-${PROJECT_VERSION}-Windows-${CMAKE_SYSTEM_PROCESSOR}-portable")
    else()
        set(CPACK_GENERATOR "TGZ")
        set(CPACK_PACKAGE_FILE_NAME "Zeus-Tools-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    include(CPack)
endif()
