#include "app_controller.h"
#include "views/action_bar.h"
#include "views/about_dialog.h"
#include "views/crypto_panel.h"
#include "views/header_bar.h"
#include "views/input_panel.h"
#include "views/result_panel.h"
#include "views/search_bar.h"
#include "views/view_context.h"

#include "components/components.h"
#include "eui/dsl.h"

#include <algorithm>
#include <memory>
#include <string>

namespace app {

namespace {

AppState& app_state = controller::state();

} // namespace

void compose(eui::Ui& ui, const eui::Screen& screen) {
    controller::initialize_documentation_scenario();
    if (!app_state.result.document) {
        app_state.result.document = std::make_shared<zeus::HighlightedDocument>(
            zeus::HighlightedDocument::plain(""));
        app_state.result.status = controller::tr(i18n::Text::WaitingForInput);
        if (!app_state.input_text.empty()) {
            controller::analyze_input(false);
        }
    }

    views::ViewContext context;
    context.dark_theme = controller::use_dark_theme();
    context.tokens = controller::theme_tokens(context.dark_theme);
    context.language_tokens = context.tokens;
    context.language_tokens.metrics.typography.body = 18.0f;
    context.language_tokens.metrics.typography.option = 18.0f;
    context.language_tokens.metrics.spacing.large = 23.0f;
    context.screen_width = screen.width;
    context.screen_height = screen.height;
    context.margin = 18.0f;
    context.header_height = 46.0f;
    context.actions_height = app_state.crypto.panel_open
        ? (app_state.crypto.hmac ? 126.0f : 88.0f)
        : 44.0f;
    context.bottom_bar_height = 42.0f;
    context.content_width = std::max(
        480.0f, screen.width - context.margin * 2.0f);
    const float available_height = std::max(
        500.0f,
        screen.height - context.margin * 2.0f - context.header_height -
            context.actions_height - context.bottom_bar_height - 26.0f);
    app_state.layout.input_ratio = std::clamp(
        app_state.layout.input_ratio, 0.30f, 0.50f);
    context.input_height = available_height * app_state.layout.input_ratio;
    context.actions_y =
        context.margin + context.header_height + context.input_height + 8.0f;
    context.result_y =
        context.actions_y + context.actions_height + 8.0f;
    context.bottom_bar_y =
        screen.height - context.margin - context.bottom_bar_height;
    context.result_height = std::max(
        220.0f, context.bottom_bar_y - context.result_y - 8.0f);
    context.header_spacer_width = std::max(
        0.0f, context.content_width - 624.0f);
    context.theme_button_center_x =
        context.margin + 297.0f + context.header_spacer_width;
    context.about_button_center_x =
        context.margin + 507.0f + context.header_spacer_width;

    ui.setFindShortcutTarget("bottom.search.hit");
    ui.setOpenShortcut([] { controller::open_input_file(); });
    ui.setSaveShortcut([] { controller::export_result(); });
    ui.setEscapeShortcut([&ui] {
        if (app_state.about_dialog_open) {
            app_state.about_dialog_open = false;
        } else if (app_state.language_dropdown_open) {
            app_state.language_dropdown_open = false;
        } else if (app_state.input_type_dropdown_open) {
            app_state.input_type_dropdown_open = false;
        } else if (app_state.csv.delimiter_dropdown_open) {
            app_state.csv.delimiter_dropdown_open = false;
        } else if (app_state.crypto.message_dropdown_open) {
            app_state.crypto.message_dropdown_open = false;
        } else if (app_state.crypto.key_encoding_dropdown_open) {
            app_state.crypto.key_encoding_dropdown_open = false;
        } else if (app_state.crypto.panel_open) {
            app_state.crypto.panel_open = false;
            app_state.crypto.hmac = false;
            app_state.crypto.message_source_index = 0;
            controller::clear_hmac_key();
            controller::clear_hmac_input_state(ui);
        } else {
            return false;
        }
        controller::request_full_repaint();
        return true;
    });

    ui.stack("root")
        .size(screen.width, screen.height)
        .content([&] {
            ui.rect("root.background")
                .size(screen.width, screen.height)
                .color(context.tokens.background)
                .dirtyKey(
                    "root.repaint." +
                    std::to_string(app_state.full_repaint_revision))
                .build();

            views::build_header_bar(ui, context);
            views::build_input_panel(ui, context);

            const float splitter_y =
                context.margin + context.header_height + context.input_height;
            const float splitter_line_width = app_state.layout.splitter_hovered
                ? 42.0f : 28.0f;
            ui.rect("workspace.splitter.line")
                .position(
                    context.margin + (context.content_width - splitter_line_width) * 0.5f,
                    splitter_y + 5.0f)
                .size(splitter_line_width, app_state.layout.splitter_hovered ? 3.0f : 2.0f)
                .color(app_state.layout.splitter_hovered
                    ? context.tokens.primary
                    : context.tokens.border)
                .radius(2.0f)
                .build();
            components::mouseArea(ui, "workspace.splitter.drag")
                .position(context.margin, splitter_y - 2.0f)
                .size(context.content_width, 12.0f)
                .zIndex(250)
                .color({0.0f, 0.0f, 0.0f, 0.0f})
                .cursor(core::CursorShape::ResizeVertical)
                .onHover([](bool hovered) {
                    if (app_state.layout.splitter_hovered == hovered) return;
                    app_state.layout.splitter_hovered = hovered;
                    controller::request_full_repaint();
                })
                .dragThreshold(0.0f)
                .onDragStart([](const components::MouseEvent&) {
                    app_state.layout.input_ratio_at_drag_start =
                        app_state.layout.input_ratio;
                })
                .onDrag([available_height](const components::MouseDragEvent& event) {
                    app_state.layout.input_ratio = std::clamp(
                        app_state.layout.input_ratio_at_drag_start +
                            event.totalY / available_height,
                        0.30f,
                        0.50f);
                    controller::request_full_repaint();
                })
                .build();

            views::build_action_bar(ui, context);
            views::build_crypto_panel(ui, context);
            views::build_result_panel(ui, context);
            views::build_search_bar(ui, context);
            views::build_about_dialog(ui, context);
        })
        .build();
}

} // namespace app
