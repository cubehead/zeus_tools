#include "action_bar.h"

#include "app_controller.h"
#include "font_tokens.h"
#include "processing_registry.h"

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

std::string action_label(processing::ActionLabel label) {
    using Label = processing::ActionLabel;
    switch (label) {
    case Label::Format: return controller::tr(i18n::Text::Format);
    case Label::Minify: return controller::tr(i18n::Text::Minify);
    case Label::Escape: return controller::tr(i18n::Text::Escape);
    case Label::Unescape: return controller::tr(i18n::Text::Unescape);
    case Label::ToYaml: return "→ YAML";
    case Label::ToXml: return "→ XML";
    case Label::ToCsv: return "→ CSV";
    case Label::ToToml: return "→ TOML";
    case Label::ToJson: return "→ JSON";
    case Label::Table: return controller::tr(i18n::Text::Table);
    case Label::Decode: return controller::tr(i18n::Text::Decode);
    case Label::Inspect: return controller::tr(i18n::Text::Inspect);
    case Label::UnixTime: return controller::tr(i18n::Text::UnixTime);
    case Label::Base64Encode: return controller::tr(i18n::Text::Base64Encode);
    case Label::UrlEncode: return controller::tr(i18n::Text::UrlEncode);
    case Label::HtmlEncode: return controller::tr(i18n::Text::HtmlEncode);
    case Label::HexEncode: return controller::tr(i18n::Text::HexEncode);
    case Label::UnicodeEncode: return controller::tr(i18n::Text::UnicodeEncode);
    case Label::Upper: return controller::tr(i18n::Text::Upper);
    case Label::Lower: return controller::tr(i18n::Text::Lower);
    }
    return {};
}
}

using namespace controller;

void build_action_bar(eui::Ui& ui, const ViewContext& context) {
    const auto& tokens = context.tokens;
    const bool dark_theme = context.dark_theme;
    const float margin = context.margin;
    const float actions_y = context.actions_y;
    const float content_width = context.content_width;
    const float actions_height = context.actions_height;
    ui.rect("actions.background")
        .position(margin, actions_y)
        .size(content_width, actions_height)
        .color(dark_theme ? eui::Color{0.070f, 0.080f, 0.098f, 1.0f}
                          : eui::Color{0.965f, 0.970f, 0.982f, 1.0f})
        .radius(7.0f)
        .border(1.0f, tokens.border)
        .build();

    constexpr float action_gap = 8.0f;
    ui.row("actions")
        .position(margin + 16.0f, actions_y + 7.0f)
        .size(content_width - 32.0f, 30.0f)
        .zIndex(200)
        .gap(action_gap)
        .alignItems(eui::Align::CENTER)
        .content([&] {
            ui.stack("actions.input.type.slot")
                .size(116.0f, 30.0f)
                .zIndex(100)
                .content([&] {
                    components::dropdown(ui, "actions.input.type")
                        .size(116.0f, 30.0f)
                        .items(input_type_items())
                        .selected(processing::input_type_index(app_state.input_type_id))
                        .open(app_state.input_type_dropdown_open)
                        .itemHeight(30.0f)
                        .zIndex(100)
                        .theme(tokens)
                        .transition(eui::Transition{})
                        .onOpenChange([](bool open) {
                            app_state.input_type_dropdown_open = open;
                            request_full_repaint();
                        })
                        .onChange([](int index) {
                            app_state.input_type_dropdown_open = false;
                            app_state.input_type_id =
                                std::string(processing::input_type_at(index).id);
                            app_state.processing_action_id = "auto";
                            analyze_input();
                        })
                        .build();
                })
                .build();

            const auto action = [&](const processing::ActionDefinition& definition) {
                const std::string_view stable_id = processing::action_id(definition);
                const bool active = app_state.processing_action_id == stable_id;
                std::string component_id = "actions." + std::string(stable_id);
                if (stable_id == "auto") {
                    component_id += "." +
                        std::to_string(static_cast<int>(definition.input_kind));
                }
                auto button = components::button(
                        ui, component_id)
                    .size(definition.width, 30.0f)
                    .text(action_label(definition.label))
                    .fontSize(20.0f)
                    .theme(tokens, active)
                    .radius(5.0f)
                    .onClick([definition] {
                        if (definition.reset_csv_options) {
                            app_state.csv.delimiter_index = 0;
                            app_state.csv.first_row_header = true;
                        }
                        app_state.processing_action_id =
                            std::string(processing::action_id(definition));
                        analyze_input();
                    });
                if (!active) {
                    button.colors(
                              {0.0f, 0.0f, 0.0f, 0.0f},
                              tokens.surfaceHover,
                              tokens.surfaceActive)
                          .textColor(tokens.text)
                          .border(0.0f, {0.0f, 0.0f, 0.0f, 0.0f})
                          .shadow(0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f});
                }
                button.build();
            };
            const std::string detected_label(processing::content_definition(
                app_state.result.detected_input_kind).compact_label);
            ui.text("actions.detected")
                .size(64.0f, 30.0f)
                .text(detected_label)
                .fontFamily(fonts::ui())
                .fontSize(18.0f)
                .fontWeight(700)
                .verticalAlign(eui::VerticalAlign::Center)
                .color(tokens.text)
                .build();

            for (const auto& definition : processing::registered_actions()) {
                if (!definition.common && processing::action_applies(
                        definition,
                        app_state.result.detected_input_kind,
                        app_state.input_text)) {
                    action(definition);
                }
            }

            if (app_state.result.detected_input_kind == zeus::ContentKind::Csv) {
                ui.stack("actions.csv.delimiter.slot")
                    .size(118.0f, 30.0f)
                    .zIndex(110)
                    .content([&] {
                        components::dropdown(ui, "actions.csv.delimiter")
                            .size(118.0f, 30.0f)
                            .items(csv_delimiter_items())
                            .selected(app_state.csv.delimiter_index)
                            .open(app_state.csv.delimiter_dropdown_open)
                            .itemHeight(30.0f)
                            .zIndex(110)
                            .theme(tokens)
                            .transition(eui::Transition{})
                            .onOpenChange([](bool open) {
                                app_state.csv.delimiter_dropdown_open = open;
                                request_full_repaint();
                            })
                            .onChange([](int index) {
                                app_state.csv.delimiter_dropdown_open = false;
                                app_state.csv.delimiter_index = index;
                                app_state.processing_action_id = "auto";
                                app_state.result.scroll = 0.0f;
                                app_state.result.csv_horizontal_scroll = 0.0f;
                                app_state.result.selected_csv_row = kNoCsvCell;
                                app_state.result.selected_csv_column = kNoCsvCell;
                                analyze_input();
                            })
                            .build();
                    })
                    .build();
                components::button(ui, "actions.csv.header")
                    .size(106.0f, 30.0f)
                    .text(std::string(tr(i18n::Text::FirstRowHeader)) + ":" +
                          (app_state.csv.first_row_header ? tr(i18n::Text::On) : tr(i18n::Text::Off)))
                    .fontSize(18.0f)
                    .theme(tokens, app_state.csv.first_row_header)
                    .radius(5.0f)
                    .onClick([] {
                        app_state.csv.first_row_header = !app_state.csv.first_row_header;
                        app_state.result.scroll = 0.0f;
                        app_state.result.selected_csv_row = kNoCsvCell;
                        app_state.result.selected_csv_column = kNoCsvCell;
                        analyze_input();
                    })
                    .build();
            }

            if (app_state.result.can_continue_decode) {
                components::button(ui, "actions.decode.again")
                    .size(94.0f, 30.0f)
                    .text(tr(i18n::Text::DecodeAgain))
                    .fontSize(18.0f)
                    .theme(tokens, false)
                    .radius(5.0f)
                    .onClick([] { continue_decode_one_layer(); })
                    .build();
            }

            if (app_state.result.detected_input_kind != zeus::ContentKind::Empty) {
                ui.rect("actions.common.divider")
                    .size(1.0f, 22.0f)
                    .color(tokens.border)
                    .build();
                for (const auto& definition : processing::registered_actions()) {
                    if (definition.common) action(definition);
                }
                components::button(ui, "actions.digest")
                    .size(90.0f, 30.0f)
                    .text(std::string(app_state.crypto.panel_open ? "▾ " : "▸ ") +
                          tr(i18n::Text::Digest))
                    .fontSize(19.0f)
                    .theme(tokens, false)
                    .radius(5.0f)
                    .colors(
                        {0.0f, 0.0f, 0.0f, 0.0f},
                        tokens.surfaceHover,
                        tokens.surfaceActive)
                    .textColor(tokens.text)
                    .border(0.0f, {0.0f, 0.0f, 0.0f, 0.0f})
                    .shadow(0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f})
                    .onClick([&ui] {
                        app_state.crypto.panel_open = !app_state.crypto.panel_open;
                        if (!app_state.crypto.panel_open) {
                            app_state.crypto.hmac = false;
                            app_state.crypto.message_source_index = 0;
                            app_state.crypto.message_dropdown_open = false;
                            clear_hmac_key();
                            clear_hmac_input_state(ui);
                        }
                        request_full_repaint();
                    })
                    .build();
            }
        })
        .build();
}

} // namespace app::views
