#include "crypto_panel.h"

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

void build_crypto_panel(eui::Ui& ui, const ViewContext& context) {
    const auto& tokens = context.tokens;
    const auto control_tokens = fonts::control_tokens(tokens);
    const float margin = context.margin;
    const float actions_y = context.actions_y;
    const float content_width = context.content_width;
    if (app_state.crypto.panel_open) {
        ui.row("actions.crypto")
            .position(margin + 16.0f, actions_y + 47.0f)
            .size(content_width - 32.0f, 34.0f)
            .gap(8.0f)
            .alignItems(eui::Align::CENTER)
            .content([&] {
                const auto algorithm_button = [&](const std::string& id,
                                                  const std::string& label,
                                                  zeus::DigestAlgorithm algorithm,
                                                  float width) {
                    components::button(ui, id)
                        .size(width, 32.0f)
                        .text(label)
                        .fontSize(fonts::button_size(18.0f))
                        .theme(tokens, false)
                        .onClick([algorithm] { compute_crypto_output(algorithm); })
                        .build();
                };

                ui.text("actions.crypto.label")
                    .size(62.0f, 30.0f)
                    .text(app_state.crypto.hmac ? tr(i18n::Text::Hmac) : tr(i18n::Text::Digest))
                    .fontFamily(fonts::ui())
                    .fontSize(17.0f)
                    .fontWeight(700)
                    .verticalAlign(eui::VerticalAlign::Center)
                    .color(tokens.text)
                    .build();
                algorithm_button("actions.crypto.md5", "MD5", zeus::DigestAlgorithm::Md5, 62.0f);
                algorithm_button("actions.crypto.sha1", "SHA-1", zeus::DigestAlgorithm::Sha1, 72.0f);
                algorithm_button("actions.crypto.sha256", "SHA-256", zeus::DigestAlgorithm::Sha256, 86.0f);
                algorithm_button("actions.crypto.sha512", "SHA-512", zeus::DigestAlgorithm::Sha512, 86.0f);

                components::button(ui, "actions.crypto.hmac")
                    .size(76.0f, 32.0f)
                    .text("HMAC")
                    .fontSize(fonts::button_size(18.0f))
                    .theme(tokens, app_state.crypto.hmac)
                    .onClick([&ui] {
                        app_state.crypto.hmac = !app_state.crypto.hmac;
                        if (!app_state.crypto.hmac) {
                            clear_hmac_key();
                            clear_hmac_input_state(ui);
                        }
                        request_full_repaint();
                    })
                    .build();
            })
            .build();

        if (app_state.crypto.hmac) {
            const float key_width = std::max(120.0f, content_width - 352.0f);
            ui.row("actions.crypto.hmac.options")
                .position(margin + 16.0f, actions_y + 85.0f)
                .size(content_width - 32.0f, 34.0f)
                .zIndex(190)
                .gap(8.0f)
                .alignItems(eui::Align::CENTER)
                .content([&] {
                    ui.stack("actions.crypto.message.slot")
                        .size(112.0f, 30.0f)
                        .zIndex(195)
                        .content([&] {
                            components::dropdown(ui, "actions.crypto.message")
                                .size(112.0f, 30.0f)
                                .items(crypto_message_items())
                                .selected(app_state.crypto.message_source_index)
                                .open(app_state.crypto.message_dropdown_open)
                                .itemHeight(30.0f)
                                .zIndex(195)
                                .theme(control_tokens)
                                .transition(eui::Transition{})
                                .onOpenChange([](bool open) {
                                    app_state.crypto.message_dropdown_open = open;
                                    request_full_repaint();
                                })
                                .onChange([](int index) {
                                    app_state.crypto.message_dropdown_open = false;
                                    app_state.crypto.message_source_index = index;
                                    request_full_repaint();
                                })
                                .build();
                        })
                        .build();

                    ui.stack("actions.crypto.encoding.slot")
                        .size(92.0f, 30.0f)
                        .zIndex(195)
                        .content([&] {
                            components::dropdown(ui, "actions.crypto.encoding")
                                .size(92.0f, 30.0f)
                                .items(hmac_key_encoding_items())
                                .selected(app_state.crypto.key_encoding_index)
                                .open(app_state.crypto.key_encoding_dropdown_open)
                                .itemHeight(30.0f)
                                .zIndex(195)
                                .theme(control_tokens)
                                .transition(eui::Transition{})
                                .onOpenChange([](bool open) {
                                    app_state.crypto.key_encoding_dropdown_open = open;
                                    request_full_repaint();
                                })
                                .onChange([](int index) {
                                    app_state.crypto.key_encoding_dropdown_open = false;
                                    app_state.crypto.key_encoding_index = index;
                                    request_full_repaint();
                                })
                                .build();
                        })
                        .build();

                    ui.stack("actions.crypto.key.slot")
                        .size(key_width, 30.0f)
                        .content([&] {
                            if (app_state.crypto.key_visible) {
                                components::input(ui, "actions.crypto.key")
                                    .size(key_width, 30.0f)
                                    .value(app_state.crypto.hmac_key)
                                    .placeholder(tr(i18n::Text::HmacKey))
                                    .fontFamily(fonts::code())
                                    .fontSize(14.0f)
                                    .theme(tokens)
                                    .onChange([](const std::string& value) {
                                        set_hmac_key(value);
                                    })
                                    .build();
                            } else {
                                auto secure_style = components::InputStyle(tokens);
                                secure_style.text = {0.0f, 0.0f, 0.0f, 0.0f};
                                components::input(ui, "actions.crypto.key")
                                    .size(key_width, 30.0f)
                                    .value(app_state.crypto.hmac_key)
                                    .placeholder(tr(i18n::Text::HmacKey))
                                    .fontFamily(fonts::code())
                                    .fontSize(14.0f)
                                    .style(secure_style)
                                    .onChange([](const std::string& value) {
                                        set_hmac_key(value);
                                    })
                                    .build();
                                if (!app_state.crypto.hmac_key.empty()) {
                                    ui.text("actions.crypto.key.mask")
                                        .position(10.0f, 0.0f)
                                        .size(std::max(0.0f, key_width - 20.0f), 30.0f)
                                        .text(std::string(app_state.crypto.hmac_key.size(), '*'))
                                        .fontFamily(fonts::code())
                                        .fontSize(14.0f)
                                        .verticalAlign(eui::VerticalAlign::Center)
                                        .color(tokens.text)
                                        .build();
                                }
                            }
                        })
                        .build();

                    components::button(ui, "actions.crypto.key.visibility")
                        .size(92.0f, 30.0f)
                        .text(app_state.crypto.key_visible
                            ? tr(i18n::Text::HideKey)
                            : tr(i18n::Text::ShowKey))
                        .fontSize(fonts::button_size(16.0f))
                        .theme(tokens, app_state.crypto.key_visible)
                        .onClick([] {
                            app_state.crypto.key_visible = !app_state.crypto.key_visible;
                            request_full_repaint();
                        })
                        .build();
                })
                .build();
        }
    }
}

} // namespace app::views
