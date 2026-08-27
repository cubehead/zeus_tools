#include "input_panel.h"

#include "app_controller.h"
#include "font_tokens.h"
#include "large_input_editor.h"

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
    if (oversized_input_paused()) {
        const float input_y = margin + header_height;
        ui.rect("input.oversized.background")
            .position(margin, input_y)
            .size(content_width, input_height)
            .color(tokens.surface)
            .radius(tokens.metrics.radius.popup)
            .border(1.0f, tokens.border)
            .build();
        ui.text("input.oversized.message")
            .position(margin + 14.0f, input_y + 12.0f)
            .size(content_width - 28.0f, 28.0f)
            .text(std::string(tr(i18n::Text::InputTooLarge)) + " · " +
                  std::to_string(app_state.input_text.size()) + " " +
                  tr(i18n::Text::Bytes))
            .fontFamily(fonts::ui())
            .fontSize(16.0f)
            .fontWeight(600)
            .verticalAlign(eui::VerticalAlign::Center)
            .color(tokens.text)
            .build();
        return;
    }
    if (lightweight_input_preview()) {
        build_large_input_editor(ui, context);
        return;
    }
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
            app_state.oversized_input_approved = false;
            app_state.large_input_page = 0;
            app_state.large_input_page_boundaries.clear();
            app_state.large_input_selection = {};
            app_state.processing_action_id = "auto";
            analyze_input(true);
        })
        .build();
}

} // namespace app::views
