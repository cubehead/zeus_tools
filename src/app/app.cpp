#include "eui_neo.h"

#include "app_controller.h"

#include "zeus/system_ui_font.h"

#include <string>
#include <vector>

namespace app {

const DslAppConfig& dslAppConfig() {
    const char* ui_font = zeus::system_ui_font_path();
    if (ui_font != nullptr && ui_font[0] != '\0') {
        core::TextPrimitive::setDefaultFontFiles(ui_font, "");
    }
    static const DslAppConfig config = DslAppConfig{}
        .title("Zeus Tools")
        .pageId("zeus_tools")
        .iconPath("assets/zeus-tools-1024.png")
        .clearColor({0.055f, 0.063f, 0.078f, 1.0f})
        .windowSize(1280, 860)
        .showDebugStatsInTitle(false)
        .fps(90.0);
    return config;
}

void onFilesDropped(const std::vector<std::string>& paths) {
    if (!paths.empty()) controller::load_input_file(paths.front());
}

} // namespace app
