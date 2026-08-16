cmake_minimum_required(VERSION 3.20)

foreach(required ZEUS_PACKAGE_ROOT ZEUS_PACKAGE_PLATFORM ZEUS_PACKAGE_VERSION)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

get_filename_component(ZEUS_PACKAGE_ROOT "${ZEUS_PACKAGE_ROOT}" ABSOLUTE)
if(NOT IS_DIRECTORY "${ZEUS_PACKAGE_ROOT}")
    message(FATAL_ERROR "Package root does not exist: ${ZEUS_PACKAGE_ROOT}")
endif()

function(require_file relative_path)
    if(NOT EXISTS "${ZEUS_PACKAGE_ROOT}/${relative_path}" OR
       IS_DIRECTORY "${ZEUS_PACKAGE_ROOT}/${relative_path}")
        message(FATAL_ERROR "Package is missing required file: ${relative_path}")
    endif()
    file(SIZE "${ZEUS_PACKAGE_ROOT}/${relative_path}" file_size)
    if(file_size EQUAL 0)
        message(FATAL_ERROR "Package contains an empty required file: ${relative_path}")
    endif()
endfunction()

foreach(file LICENSE PRIVACY.md THIRD_PARTY_NOTICES.md)
    require_file("${file}")
endforeach()

set(required_licenses
    EUI-NEO-APACHE-2.0.txt
    FONT-AWESOME-FREE.txt
    FREETYPE.txt
    GLFW.md
    LIBPNG.txt
    MD4C-MIT.md
    OCTICONS-MIT.txt
    PUGIXML-MIT.md
    TRAY-MIT.txt
    YAML-CPP-MIT.txt
    YYJSON-MIT.txt
    ZLIB.txt
    tomlplusplus.txt
)
foreach(license IN LISTS required_licenses)
    require_file("licenses/${license}")
endforeach()

if(ZEUS_PACKAGE_PLATFORM STREQUAL "windows")
    foreach(file
            ZeusTools.exe
            zeus-tools-cli.exe
            "assets/Font Awesome 7 Free-Solid-900.otf"
            assets/github-mark.svg
            assets/zeus-tools-1024.png)
        require_file("${file}")
    endforeach()
elseif(ZEUS_PACKAGE_PLATFORM STREQUAL "macos")
    set(app "Zeus Tools.app/Contents")
    foreach(file
            "${app}/Info.plist"
            "${app}/MacOS/Zeus Tools"
            "${app}/MacOS/assets/Font Awesome 7 Free-Solid-900.otf"
            "${app}/MacOS/assets/github-mark.svg"
            "${app}/MacOS/assets/zeus-tools-1024.png"
            "${app}/Resources/zeus-tools.icns"
            zeus-tools-cli)
        require_file("${file}")
    endforeach()
    file(READ "${ZEUS_PACKAGE_ROOT}/${app}/Info.plist" plist)
    foreach(expected
            "<string>io.github.zeustools.app</string>"
            "<string>${ZEUS_PACKAGE_VERSION}</string>"
            "<string>zeus-tools.icns</string>"
            "<true/>")
        string(FIND "${plist}" "${expected}" found)
        if(found EQUAL -1)
            message(FATAL_ERROR "Info.plist is missing expected value: ${expected}")
        endif()
    endforeach()
else()
    message(FATAL_ERROR "Unsupported package platform: ${ZEUS_PACKAGE_PLATFORM}")
endif()

file(GLOB_RECURSE packaged_files RELATIVE "${ZEUS_PACKAGE_ROOT}"
    "${ZEUS_PACKAGE_ROOT}/*")
set(forbidden_names
    .DS_Store
    JingNanJunJunTi-JinNanJunJunTi-Bold-2.ttf
    YouSheBiaoTiHei-2.ttf
    icon.ico
    icon.png
    icon.svg
    mona-loading-default.gif
)
foreach(path IN LISTS packaged_files)
    get_filename_component(name "${path}" NAME)
    if(name IN_LIST forbidden_names)
        message(FATAL_ERROR "Package contains forbidden development/demo asset: ${path}")
    endif()
endforeach()

message(STATUS
    "Validated Zeus Tools ${ZEUS_PACKAGE_VERSION} ${ZEUS_PACKAGE_PLATFORM} package")
