#include "large_input_editor.h"

#include "app_controller.h"
#include "font_tokens.h"
#include "large_input_paging.h"

#include "components/components.h"
#include "components/input_model.h"

#include <algorithm>
#include <string>

namespace app::views {

namespace {
AppState& app_state = controller::state();

void prepare_page_state(
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

void ensure_page_boundaries() {
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
} // namespace

void build_large_input_editor(eui::Ui& ui, const ViewContext& context) {
    using namespace controller;
    const auto& tokens = context.tokens;
    const float margin = context.margin;
    const float input_y = margin + context.header_height;
    constexpr float pager_height = 34.0f;

    ensure_page_boundaries();
    const std::size_t pages = app_state.large_input_page_boundaries.size() - 1;
    app_state.large_input_page = std::min(app_state.large_input_page, pages - 1);
    const large_input::PageRange range{
        app_state.large_input_page_boundaries[app_state.large_input_page],
        app_state.large_input_page_boundaries[app_state.large_input_page + 1],
    };
    const std::string page_text = app_state.input_text.substr(
        range.start, range.end - range.start);
    const std::string editor_id = "input.large.editor." +
        std::to_string(app_state.large_input_page);
    prepare_page_state(ui, editor_id, page_text);

    components::input(ui, editor_id)
        .position(margin, input_y)
        .size(context.content_width,
              std::max(60.0f, context.input_height - pager_height))
        .value(page_text)
        .multiline(true)
        .fontFamily(fonts::code())
        .fontSize(14.5f)
        .theme(tokens)
        .onChange([range](const std::string& value) {
            const std::size_t old_size = range.end - range.start;
            app_state.input_text.replace(range.start, old_size, value);
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

    const float pager_y = input_y + context.input_height - pager_height + 3.0f;
    ui.row("input.large.pager")
        .position(margin + 10.0f, pager_y)
        .size(context.content_width - 20.0f, 28.0f)
        .gap(8.0f)
        .alignItems(eui::Align::CENTER)
        .content([&] {
            components::button(ui, "input.large.previous")
                .size(38.0f, 26.0f)
                .text("‹")
                .fontSize(fonts::button_size(22.0f))
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
                .fontSize(fonts::button_size(22.0f))
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
                .size(std::max(180.0f, context.content_width - 158.0f), 26.0f)
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
            components::button(ui, "input.large.copy.all")
                .size(40.0f, 26.0f)
                .text("")
                .icon(0xF0C5)
                .iconSize(15.0f)
                .theme(tokens, false)
                .radius(5.0f)
                .onClick([] { copy_input_text(); })
                .build();
        })
        .build();

    components::tooltip(ui, "input.large.previous.tooltip")
        .source("input.large.previous.bg")
        .value(tr(i18n::Text::PreviousPage))
        .anchor(margin + 28.0f, pager_y + 29.0f)
        .bounds(context.screen_width, context.screen_height)
        .theme(tokens)
        .zIndex(1000)
        .build();
    components::tooltip(ui, "input.large.next.tooltip")
        .source("input.large.next.bg")
        .value(tr(i18n::Text::NextPage))
        .anchor(margin + 74.0f, pager_y + 29.0f)
        .bounds(context.screen_width, context.screen_height)
        .theme(tokens)
        .zIndex(1000)
        .build();
    components::tooltip(ui, "input.large.copy.tooltip")
        .source("input.large.copy.all.bg")
        .value(tr(i18n::Text::CopyInput))
        .anchor(margin + context.content_width - 24.0f, pager_y + 29.0f)
        .bounds(context.screen_width, context.screen_height)
        .theme(tokens)
        .zIndex(1000)
        .build();
}

} // namespace app::views
