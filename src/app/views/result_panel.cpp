#include "result_panel.h"

#include "app_controller.h"
#include "font_tokens.h"
#include "csv_table_view.h"
#include "rich_text_view.h"

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

void build_result_panel(eui::Ui& ui, const ViewContext& context) {
    const auto& tokens = context.tokens;
    const bool dark_theme = context.dark_theme;
    const float margin = context.margin;
    const float result_y = context.result_y;
    const float content_width = context.content_width;
    const float result_height = context.result_height;
    ui.rect("result.background")
        .position(margin, result_y)
        .size(content_width, result_height)
        .color(dark_theme ? eui::Color{0.075f, 0.086f, 0.106f, 1.0f}
                          : eui::Color{0.985f, 0.988f, 0.995f, 1.0f})
        .radius(8.0f)
        .border(1.0f, tokens.border)
        .build();

    const auto document = app_state.result.document;
    constexpr float row_height = 25.0f;
    if (app_state.input_text.empty()) {
        ui.text("result.empty.placeholder")
            .position(margin + 20.0f, result_y + 16.0f)
            .size(content_width - 40.0f, 28.0f)
            .text(tr(i18n::Text::WaitingForInput))
            .fontFamily(fonts::ui())
            .fontSize(15.0f)
            .verticalAlign(eui::VerticalAlign::Center)
            .color(dark_theme
                ? eui::Color{0.48f, 0.52f, 0.60f, 1.0f}
                : eui::Color{0.45f, 0.48f, 0.55f, 1.0f})
            .build();
    } else if (app_state.result.csv && !app_state.result.csv->rows.empty()) {
        zeus::app_components::csvTableView(ui, "result.csv.table")
            .position(margin + 8.0f, result_y + 6.0f)
            .size(content_width - 16.0f, result_height - 12.0f)
            .document(app_state.result.csv)
            .scroll(app_state.result.scroll, app_state.result.csv_horizontal_scroll)
            .selection(app_state.result.selected_csv_row, app_state.result.selected_csv_column)
            .firstRowHeader(app_state.csv.first_row_header)
            .theme(tokens)
            .searchMatches(app_state.search.csv_matches, app_state.search.active_match)
            .onCopy([] { copy_result(true); })
            .build();
    } else if (!app_state.result.image_preview_source.empty()) {
        const float inner_x = margin + 8.0f;
        const float inner_width = content_width - 16.0f;
        const float inner_height = result_height - 12.0f;
        const float summary_height = std::clamp(inner_height * 0.24f, 76.0f, 124.0f);
        const float preview_height = std::max(60.0f, inner_height - summary_height - 8.0f);

        ui.rect("result.image.preview.background")
            .position(inner_x, result_y + 6.0f)
            .size(inner_width, preview_height)
            .color(dark_theme ? eui::Color{0.045f, 0.052f, 0.064f, 1.0f}
                              : eui::Color{0.94f, 0.95f, 0.97f, 1.0f})
            .radius(6.0f)
            .build();
        ui.image("result.image.preview")
            .position(inner_x + 10.0f, result_y + 16.0f)
            .size(inner_width - 20.0f, std::max(40.0f, preview_height - 20.0f))
            .source(app_state.result.image_preview_source)
            .contain()
            .build();
        zeus::app_components::richTextView(ui, "result.document")
            .position(inner_x, result_y + 14.0f + preview_height)
            .size(inner_width, summary_height)
            .document(document)
            .selection(app_state.result.selection)
            .folds(app_state.result.folds)
            .scroll(app_state.result.scroll)
            .rowHeight(row_height)
            .dark(dark_theme)
            .theme(tokens)
            .searchMatches(app_state.search.document_matches, app_state.search.active_match)
            .onCopy([] { copy_result(true); })
            .build();
    } else {
        zeus::app_components::richTextView(ui, "result.document")
            .position(margin + 8.0f, result_y + 6.0f)
            .size(content_width - 16.0f, result_height - 12.0f)
            .document(document)
            .selection(app_state.result.selection)
            .folds(app_state.result.folds)
            .scroll(app_state.result.scroll)
            .rowHeight(row_height)
            .dark(dark_theme)
            .theme(tokens)
            .searchMatches(app_state.search.document_matches, app_state.search.active_match)
            .onCopy([] { copy_result(true); })
            .build();
    }
}

} // namespace app::views
