#include "zeus/system_ui_font.h"

extern "C" const char* zeus_system_ui_font_path_native();

namespace zeus {

const char* system_ui_font_path() {
    return zeus_system_ui_font_path_native();
}

} // namespace zeus
