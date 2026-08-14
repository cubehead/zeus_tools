#include "input_panel.h"

#include "app_controller.h"
#include "font_tokens.h"

#include "components/components.h"
#include "core/platform/platform.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace app::views {

namespace {
AppState& app_state = controller::state();
}

using namespace controller;

void build_input_panel(eui::Ui& ui, const ViewContext& context) {
    const auto& tokens = context.tokens;
    const float margin = context.margin;
    const float header_height = context.header_height;
    const float content_width = context.content_width;
    const float input_height = context.input_height;
    components::input(ui, "input.editor")
        .position(margin, margin + header_height)
        .size(content_width, input_height)
        .value(app_state.input_text)
        .placeholder(tr(i18n::Text::PasteOrTypeJson))
        .multiline(true)
        .fontFamily(fonts::code())
        .fontSize(14.5f)
        .theme(tokens)
        .onChange([](const std::string& value) {
            app_state.input_text = value;
            app_state.processing_mode_index = 0;
            analyze_input(true);
        })
        .build();
}

} // namespace app::views
