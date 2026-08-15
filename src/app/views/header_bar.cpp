#include "header_bar.h"

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

void build_header_bar(eui::Ui& ui, const ViewContext& context) {
    const auto& tokens = context.tokens;
    const auto& language_tokens = context.language_tokens;
    const float margin = context.margin;
    const float content_width = context.content_width;
    const float header_height = context.header_height;
    const float header_spacer_width = context.header_spacer_width;
    const float theme_button_center_x = context.theme_button_center_x;
    ui.row("header")
        .position(margin, margin)
        .size(content_width, header_height)
        .zIndex(300)
        .gap(8.0f)
        .alignItems(eui::Align::CENTER)
        .content([&] {
            ui.text("header.title")
                .size(160.0f, header_height)
                .text("Zeus Tools")
                .fontFamily(fonts::ui())
                .fontSize(22.0f)
                .fontWeight(800)
                .verticalAlign(eui::VerticalAlign::Center)
                .color(tokens.text)
                .build();

            ui.rect("header.spacer")
                .size(header_spacer_width, 1.0f)
                .color({0.0f, 0.0f, 0.0f, 0.0f})
                .build();

            components::button(ui, "header.open")
                .size(42.0f, 38.0f)
                .text("")
                .icon(0xF07C)
                .iconSize(18.0f)
                .theme(tokens, false)
                .onClick([] { open_input_file(); })
                .build();

            components::button(ui, "header.export")
                .size(42.0f, 38.0f)
                .text("")
                .icon(0xF56E)
                .iconSize(18.0f)
                .theme(tokens, false)
                .onClick([] { export_result(); })
                .build();

            components::button(ui, "header.theme")
                .size(42.0f, 38.0f)
                .text("")
                .icon(theme_icon())
                .iconSize(18.0f)
                .theme(tokens, false)
                .onClick([] {
                    app_state.theme_preference_index = (app_state.theme_preference_index + 1) % 3;
                    zeus::save_theme_preference(
                        static_cast<zeus::ThemePreference>(app_state.theme_preference_index));
                    request_full_repaint();
                })
                .build();

            ui.stack("header.language.slot")
                .size(152.0f, 38.0f)
                .zIndex(310)
                .content([&] {
                    components::dropdown(ui, "header.language")
                        .size(152.0f, 38.0f)
                        .items(language_items())
                        .selected(app_state.locale_preference_index)
                        .open(app_state.language_dropdown_open)
                        .itemHeight(38.0f)
                        .zIndex(310)
                        .theme(language_tokens)
                        .transition(eui::Transition{})
                        .onOpenChange([](bool open) {
                            app_state.language_dropdown_open = open;
                            request_full_repaint();
                        })
                        .onChange([](int index) {
                            app_state.language_dropdown_open = false;
                            app_state.locale_preference_index = index;
                            zeus::save_locale_preference(
                                static_cast<zeus::LocalePreference>(index));
                            analyze_input();
                            core::platform::requestUiUpdate();
                        })
                        .build();
                })
                .build();

            components::button(ui, "header.about")
                .size(42.0f, 38.0f)
                .text("")
                .icon(0xF05A)
                .iconSize(18.0f)
                .theme(tokens, false)
                .onClick([] {
                    app_state.language_dropdown_open = false;
                    app_state.about_dialog_open = true;
                    request_full_repaint();
                })
                .build();

            components::button(ui, "header.clear")
                .size(88.0f, 38.0f)
                .text(tr(i18n::Text::Clear))
                .fontSize(20.0f)
                .theme(tokens, false)
                .onClick([&ui] {
                    app_state.input_text.clear();
                    app_state.search.query.clear();
                    app_state.crypto.hmac_key.clear();
                    app_state.processing_action_id = "auto";
                    app_state.input_type_id = "auto";
                    app_state.input_type_dropdown_open = false;
                    app_state.csv.delimiter_index = 0;
                    app_state.csv.delimiter_dropdown_open = false;
                    app_state.csv.first_row_header = true;
                    app_state.crypto.panel_open = false;
                    app_state.crypto.hmac = false;
                    app_state.crypto.message_source_index = 0;
                    app_state.crypto.message_dropdown_open = false;
                    clear_hmac_key();
                    clear_hmac_input_state(ui);
                    app_state.result.value.clear();
                    app_state.result.decode_chain.clear();
                    app_state.result.can_continue_decode = false;
                    analyze_input();
                })
                .build();

        })
        .build();

    components::tooltip(ui, "header.theme.tooltip")
        .source("header.theme.bg")
        .value(theme_tooltip())
        .anchor(theme_button_center_x, margin + header_height - 2.0f)
        .bounds(context.screen_width, context.screen_height)
        .theme(tokens)
        .zIndex(1000)
        .build();

    components::tooltip(ui, "header.open.tooltip")
        .source("header.open.bg")
        .value(tr(i18n::Text::OpenFile))
        .anchor(theme_button_center_x - 100.0f, margin + header_height - 2.0f)
        .bounds(context.screen_width, context.screen_height)
        .theme(tokens)
        .zIndex(1000)
        .build();

    components::tooltip(ui, "header.export.tooltip")
        .source("header.export.bg")
        .value(tr(i18n::Text::ExportResult))
        .anchor(theme_button_center_x - 50.0f, margin + header_height - 2.0f)
        .bounds(context.screen_width, context.screen_height)
        .theme(tokens)
        .zIndex(1000)
        .build();

    components::tooltip(ui, "header.about.tooltip")
        .source("header.about.bg")
        .value(tr(i18n::Text::About))
        .anchor(context.about_button_center_x, margin + header_height - 2.0f)
        .bounds(context.screen_width, context.screen_height)
        .theme(tokens)
        .zIndex(1000)
        .build();
}

} // namespace app::views
