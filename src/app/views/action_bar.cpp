#include "action_bar.h"

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
                        .selected(app_state.input_type_override_index)
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
                            app_state.input_type_override_index = index;
                            app_state.processing_mode_index = 0;
                            analyze_input();
                        })
                        .build();
                })
                .build();

            const auto action = [&](const std::string& id,
                                    const std::string& label,
                                    int index,
                                    float width) {
                const bool active = app_state.processing_mode_index == index;
                auto button = components::button(ui, id)
                    .size(width, 30.0f)
                    .text(label)
                    .fontSize(20.0f)
                    .theme(tokens, active)
                    .radius(5.0f)
                    .onClick([index] {
                        if (index == 15) {
                            app_state.csv.delimiter_index = 0;
                            app_state.csv.first_row_header = true;
                        }
                        app_state.processing_mode_index = index;
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
            std::string detected_label = zeus::content_kind_name(app_state.result.detected_input_kind);
            if (app_state.result.detected_input_kind == zeus::ContentKind::UrlEncoded) detected_label = "URL";
            if (app_state.result.detected_input_kind == zeus::ContentKind::JsonEscaped) {
                detected_label = "Esc JSON";
            }
            ui.text("actions.detected")
                .size(64.0f, 30.0f)
                .text(detected_label)
                .fontFamily(fonts::ui())
                .fontSize(18.0f)
                .fontWeight(700)
                .verticalAlign(eui::VerticalAlign::Center)
                .color(tokens.text)
                .build();

            if (app_state.result.detected_input_kind == zeus::ContentKind::JsonEscaped) {
                action("actions.json.unescape", tr(i18n::Text::Unescape), 0, 76.0f);
            } else if (app_state.result.detected_input_kind == zeus::ContentKind::Json) {
                action("actions.format", tr(i18n::Text::Format), 0, 64.0f);
                action("actions.minify", tr(i18n::Text::Minify), 1, 62.0f);
                action("actions.escape", tr(i18n::Text::Escape), 6, 62.0f);
                action("actions.json.to_yaml", "→ YAML", 11, 68.0f);
                action("actions.json.to_xml", "→ XML", 13, 62.0f);
                action("actions.json.to_csv", "→ CSV", 15, 62.0f);
            } else if (app_state.result.detected_input_kind == zeus::ContentKind::Xml) {
                action("actions.xml.format", tr(i18n::Text::Format), 9, 64.0f);
                action("actions.xml.to_json", "→ JSON", 16, 68.0f);
            } else if (app_state.result.detected_input_kind == zeus::ContentKind::Yaml) {
                action("actions.yaml.format", tr(i18n::Text::Format), 10, 64.0f);
                action("actions.yaml.to_json", "→ JSON", 12, 68.0f);
            } else if (app_state.result.detected_input_kind == zeus::ContentKind::Csv) {
                action("actions.table", tr(i18n::Text::Table), 0, 60.0f);
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
                                app_state.processing_mode_index = 0;
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
            if (app_state.result.detected_input_kind == zeus::ContentKind::Base64) {
                action("actions.base64.decode", tr(i18n::Text::Decode), 0, 64.0f);
            } else if (app_state.result.detected_input_kind == zeus::ContentKind::UrlEncoded) {
                action("actions.url.decode", tr(i18n::Text::Decode), 0, 64.0f);
            } else if (app_state.result.detected_input_kind == zeus::ContentKind::Jwt) {
                action("actions.jwt.inspect", tr(i18n::Text::Inspect), 0, 64.0f);
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
                action("actions.base64.encode", tr(i18n::Text::Base64Encode), 2, 104.0f);
                action("actions.url.encode", tr(i18n::Text::UrlEncode), 3, 98.0f);
                action("actions.upper", tr(i18n::Text::Upper), 7, 56.0f);
                action("actions.lower", tr(i18n::Text::Lower), 8, 56.0f);
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
