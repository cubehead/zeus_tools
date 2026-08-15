#include "search_bar.h"

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

void build_search_bar(eui::Ui& ui, const ViewContext& context) {
    const auto& tokens = context.tokens;
    const bool dark_theme = context.dark_theme;
    const float margin = context.margin;
    const float content_width = context.content_width;
    const float bottom_bar_y = context.bottom_bar_y;
    const float bottom_bar_height = context.bottom_bar_height;
    const std::string visible_issue = app_state.result.issue.empty()
        ? app_state.search.issue : app_state.result.issue;
    const std::string visible_issue_detail = app_state.result.issue.empty()
        ? app_state.search.issue_detail : app_state.result.issue_detail;
    const std::string footer = visible_issue.empty()
        ? app_state.result.status
        : app_state.result.status + " · " + visible_issue;
    const bool csv_cell_selected = app_state.result.csv &&
        app_state.result.selected_csv_row < app_state.result.csv->rows.size() &&
        app_state.result.selected_csv_column <
            app_state.result.csv->rows[app_state.result.selected_csv_row].size();
    const std::size_t selected_bytes = csv_cell_selected
        ? app_state.result.csv->rows[app_state.result.selected_csv_row]
              [app_state.result.selected_csv_column].size()
        : app_state.result.document ? app_state.result.selection.selected_length() : 0;
    const std::string selection_status = csv_cell_selected
        ? " · R" + std::to_string(app_state.result.selected_csv_row + 1) +
              " C" + std::to_string(app_state.result.selected_csv_column + 1) + " · " +
              tr(i18n::Text::Selected) + " " + std::to_string(selected_bytes) +
              " " + tr(i18n::Text::Bytes)
        : selected_bytes > 0
            ? " · " + std::string(tr(i18n::Text::Selected)) + " " +
                  std::to_string(selected_bytes) + " " + tr(i18n::Text::Bytes)
            : std::string{};
    const std::size_t total_matches = app_state.result.csv
        ? app_state.search.csv_matches.size() : app_state.search.document_matches.size();
    const std::string count = total_matches == 0
        ? "0 / 0"
        : std::to_string(app_state.search.active_match + 1) + " / " +
              std::to_string(total_matches);
    const float bottom_inner_width = content_width - 12.0f;
    const float search_width = std::clamp(content_width * 0.35f, 150.0f, 380.0f);
    const float status_width = std::max(52.0f, bottom_inner_width - search_width - 361.0f);
#if defined(__APPLE__)
    const std::string search_placeholder =
        std::string(tr(i18n::Text::SearchResult)) + "  ·  ⌘F";
#else
    const std::string search_placeholder =
        std::string(tr(i18n::Text::SearchResult)) + "  ·  Ctrl+F";
#endif

    ui.rect("bottom.background")
        .position(margin, bottom_bar_y)
        .size(content_width, bottom_bar_height)
        .color(dark_theme ? eui::Color{0.070f, 0.080f, 0.098f, 1.0f}
                          : eui::Color{0.965f, 0.970f, 0.982f, 1.0f})
        .radius(7.0f)
        .border(1.0f, tokens.border)
        .build();

    ui.row("bottom.controls")
        .position(margin + 6.0f, bottom_bar_y + 4.0f)
        .size(bottom_inner_width, bottom_bar_height - 8.0f)
        .gap(5.0f)
        .alignItems(eui::Align::CENTER)
        .content([&] {
            ui.text("bottom.status")
                .size(status_width, 34.0f)
                .text(footer + selection_status)
                .fontFamily(fonts::ui())
                .fontSize(14.0f)
                .interactive(!visible_issue_detail.empty())
                .cursor(core::CursorShape::Arrow)
                .verticalAlign(eui::VerticalAlign::Center)
                .color(visible_issue.empty()
                    ? (dark_theme ? eui::Color{0.55f, 0.68f, 0.82f, 1.0f}
                                  : eui::Color{0.18f, 0.40f, 0.66f, 1.0f})
                    : (dark_theme ? eui::Color{0.96f, 0.45f, 0.45f, 1.0f}
                                  : eui::Color{0.72f, 0.12f, 0.16f, 1.0f}))
                .build();

            components::input(ui, "bottom.search")
                .size(search_width, 34.0f)
                .value(app_state.search.query)
                .placeholder(search_placeholder)
                .fontFamily(fonts::ui())
                .theme(tokens)
                .onChange([](const std::string& value) {
                    app_state.search.query = value;
                    update_search();
                })
                .build();

            components::button(ui, "bottom.search.case")
                .size(42.0f, 34.0f)
                .text("Aa")
                .fontSize(16.0f)
                .theme(tokens, app_state.search.case_sensitive)
                .onClick([] {
                    app_state.search.case_sensitive = !app_state.search.case_sensitive;
                    update_search();
                })
                .build();

            components::button(ui, "bottom.search.regex")
                .size(42.0f, 34.0f)
                .text(".*")
                .fontSize(16.0f)
                .theme(tokens, app_state.search.use_regex)
                .onClick([] {
                    app_state.search.use_regex = !app_state.search.use_regex;
                    update_search();
                })
                .build();

            components::button(ui, "bottom.previous")
                .size(34.0f, 34.0f)
                .text("↑")
                .fontSize(19.0f)
                .theme(tokens, false)
                .onClick([] { move_match(-1); })
                .build();

            components::button(ui, "bottom.next")
                .size(34.0f, 34.0f)
                .text("↓")
                .fontSize(19.0f)
                .theme(tokens, false)
                .onClick([] { move_match(1); })
                .build();

            ui.text("bottom.match.count")
                .size(58.0f, 34.0f)
                .text(count)
                .fontSize(14.0f)
                .verticalAlign(eui::VerticalAlign::Center)
                .color(dark_theme ? eui::Color{0.64f, 0.69f, 0.78f, 1.0f}
                                  : eui::Color{0.38f, 0.42f, 0.50f, 1.0f})
                .build();

            components::button(ui, "bottom.copy")
                .size(116.0f, 34.0f)
                .text((app_state.result.csv ? !csv_cell_selected : app_state.result.selection.empty())
                    ? tr(i18n::Text::CopyAll)
                    : tr(i18n::Text::CopySelected))
                .fontSize(18.0f)
                .theme(tokens, false)
                .onClick([] { copy_result(false); })
                .build();
        })
        .build();

    if (!visible_issue_detail.empty()) {
        const float detail_width = std::min(560.0f, content_width - 12.0f);
        constexpr float detail_height = 76.0f;
        ui.stack("bottom.issue.detail")
            .position(margin + 6.0f, bottom_bar_y - detail_height - 6.0f)
            .size(detail_width, detail_height)
            .zIndex(1200)
            .hoverOpacityFrom("bottom.status")
            .content([&] {
                ui.rect("bottom.issue.detail.bg")
                    .size(detail_width, detail_height)
                    .color(dark_theme
                        ? eui::Color{0.105f, 0.115f, 0.140f, 0.98f}
                        : eui::Color{1.0f, 1.0f, 1.0f, 0.98f})
                    .radius(8.0f)
                    .border(1.0f, tokens.border)
                    .shadow(12.0f, 0.0f, 4.0f, {0.0f, 0.0f, 0.0f, 0.20f})
                    .build();
                ui.text("bottom.issue.detail.text")
                    .position(12.0f, 8.0f)
                    .size(detail_width - 24.0f, detail_height - 16.0f)
                    .text(visible_issue_detail)
                    .fontFamily(fonts::ui())
                    .fontSize(14.0f)
                    .lineHeight(19.0f)
                    .wrap(true)
                    .verticalAlign(eui::VerticalAlign::Center)
                    .color(tokens.text)
                    .build();
            })
            .build();
    }
}

} // namespace app::views
