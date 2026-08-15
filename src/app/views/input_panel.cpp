#include "input_panel.h"

#include "app_controller.h"
#include "font_tokens.h"
#include "large_input_paging.h"

#include "components/components.h"
#include "components/input_model.h"
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

void prepare_large_page_state(
    eui::Ui& ui,
    const std::string& id,
    const std::string& page_text) {
    using InputState = components::input_detail::InputModel::InputState;
    InputState& state = ui.state<InputState>(id);
    if (state.text == page_text) return;
    state.text = page_text;
    state.compositionText.clear();
    state.cursor = 0;
    state.selectionStart = 0;
    state.selectionEnd = 0;
    state.dragAnchor = 0;
    state.horizontalScroll = 0.0f;
    state.verticalScroll = 0.0f;
    ++state.textRevision;
    ++state.compositionRevision;
    state.layoutCacheValid = false;
    state.undoStack.clear();
    state.redoStack.clear();
}

void ensure_large_input_page_boundaries() {
    if (!app_state.large_input_page_boundaries.empty() &&
        app_state.large_input_page_boundaries.back() == app_state.input_text.size()) {
        return;
    }
    app_state.large_input_page_boundaries =
        large_input::page_boundaries(app_state.input_text);
    app_state.large_input_page = std::min(
        app_state.large_input_page,
        app_state.large_input_page_boundaries.size() - 2);
}
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
        ensure_large_input_page_boundaries();
        const std::size_t pages = app_state.large_input_page_boundaries.size() - 1;
        app_state.large_input_page = std::min(app_state.large_input_page, pages - 1);
        const large_input::PageRange range{
            app_state.large_input_page_boundaries[app_state.large_input_page],
            app_state.large_input_page_boundaries[app_state.large_input_page + 1],
        };
        const std::string page_text = app_state.input_text.substr(
            range.start, range.end - range.start);
        constexpr float pager_height = 34.0f;
        const std::string editor_id = "input.large.editor." +
            std::to_string(app_state.large_input_page);
        prepare_large_page_state(ui, editor_id, page_text);
        components::input(ui, editor_id)
            .position(margin, margin + header_height)
            .size(content_width, std::max(60.0f, input_height - pager_height))
            .value(page_text)
            .multiline(true)
            .fontFamily(fonts::code())
            .fontSize(14.5f)
            .theme(tokens)
            .onChange([range](const std::string& value) {
                const std::size_t old_size = range.end - range.start;
                app_state.input_text.replace(
                    range.start, old_size, value);
                large_input::resize_page(
                    app_state.large_input_page_boundaries,
                    app_state.large_input_page,
                    value.size());
                if (value.size() > large_input::kMaxInteractivePageBytes) {
                    app_state.large_input_page_boundaries =
                        large_input::page_boundaries(app_state.input_text);
                    app_state.large_input_page = std::min(
                        range.start / large_input::kPageBytes,
                        app_state.large_input_page_boundaries.size() - 2);
                }
                app_state.oversized_input_approved = false;
                app_state.processing_action_id = "auto";
                analyze_input(true);
            })
            .build();

        ui.row("input.large.pager")
            .position(margin + 10.0f,
                      margin + header_height + input_height - pager_height + 3.0f)
            .size(content_width - 20.0f, 28.0f)
            .gap(8.0f)
            .alignItems(eui::Align::CENTER)
            .content([&] {
                components::button(ui, "input.large.previous")
                    .size(38.0f, 26.0f)
                    .text("‹")
                    .fontSize(22.0f)
                    .theme(tokens, false)
                    .radius(5.0f)
                    .onClick([] {
                        if (app_state.large_input_page > 0) {
                            --app_state.large_input_page;
                            request_full_repaint();
                        }
                    })
                    .build();
                components::button(ui, "input.large.next")
                    .size(38.0f, 26.0f)
                    .text("›")
                    .fontSize(22.0f)
                    .theme(tokens, false)
                    .radius(5.0f)
                    .onClick([pages] {
                        if (app_state.large_input_page + 1 < pages) {
                            ++app_state.large_input_page;
                            request_full_repaint();
                        }
                    })
                    .build();
                ui.text("input.large.page.info")
                    .size(std::max(220.0f, content_width - 110.0f), 26.0f)
                    .text(std::string(tr(i18n::Text::LargeInputEditor)) + " · " +
                          std::to_string(app_state.large_input_page + 1) + "/" +
                          std::to_string(pages) + " · " +
                          std::to_string(app_state.input_text.size()) + " " +
                          tr(i18n::Text::Bytes))
                    .fontFamily(fonts::ui())
                    .fontSize(15.0f)
                    .fontWeight(600)
                    .verticalAlign(eui::VerticalAlign::Center)
                    .color(tokens.text)
                    .build();
            })
            .build();
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
            app_state.processing_action_id = "auto";
            analyze_input(true);
        })
        .build();
}

} // namespace app::views
