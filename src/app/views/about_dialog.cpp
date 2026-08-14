#include "about_dialog.h"

#include "app_controller.h"
#include "font_tokens.h"

#include "components/components.h"
#include "eui/platform.h"

#include <algorithm>
#include <string>

#ifndef ZEUS_APP_VERSION
#define ZEUS_APP_VERSION "dev"
#endif

#ifndef ZEUS_APP_BUILD_NUMBER
#define ZEUS_APP_BUILD_NUMBER "local"
#endif

namespace app::views {

namespace {

AppState& app_state = controller::state();
constexpr const char* kProjectUrl = "https://github.com/cubehead/zeus_tools";

} // namespace

void build_about_dialog(eui::Ui& ui, const ViewContext& context) {
    if (!app_state.about_dialog_open) return;

    const auto& tokens = context.tokens;
    const float dialog_width = std::min(570.0f, context.screen_width - 40.0f);
    const float dialog_height = std::min(410.0f, context.screen_height - 40.0f);
    const float dialog_x = (context.screen_width - dialog_width) * 0.5f;
    const float dialog_y = (context.screen_height - dialog_height) * 0.5f;
    const std::string version = std::string(controller::tr(i18n::Text::Version)) +
        " " ZEUS_APP_VERSION "  ·  " + controller::tr(i18n::Text::Build) +
        " " ZEUS_APP_BUILD_NUMBER;

    components::mouseArea(ui, "about.backdrop")
        .position(0.0f, 0.0f)
        .size(context.screen_width, context.screen_height)
        .zIndex(2000)
        .color({0.0f, 0.0f, 0.0f, context.dark_theme ? 0.58f : 0.36f})
        .onTap([] {
            app_state.about_dialog_open = false;
            controller::request_full_repaint();
        })
        .build();

    ui.stack("about.dialog")
        .position(dialog_x, dialog_y)
        .size(dialog_width, dialog_height)
        .zIndex(2100)
        .content([&] {
            ui.rect("about.dialog.background")
                .size(dialog_width, dialog_height)
                .color(tokens.surface)
                .radius(18.0f)
                .border(1.0f, tokens.border)
                .shadow(28.0f, 0.0f, 10.0f, {0.0f, 0.0f, 0.0f, 0.32f})
                .build();

            ui.image("about.logo")
                .position(28.0f, 28.0f)
                .size(82.0f, 82.0f)
                .source("assets/zeus-tools-1024.png")
                .radius(18.0f)
                .cover()
                .build();

            ui.text("about.title")
                .position(132.0f, 30.0f)
                .size(dialog_width - 196.0f, 38.0f)
                .text("Zeus Tools")
                .fontFamily(fonts::ui())
                .fontSize(28.0f)
                .fontWeight(800)
                .color(tokens.text)
                .build();

            ui.text("about.version")
                .position(132.0f, 72.0f)
                .size(dialog_width - 196.0f, 26.0f)
                .text(version)
                .fontFamily(fonts::code())
                .fontSize(15.0f)
                .color(components::theme::withAlpha(tokens.text, 0.66f))
                .build();

            components::button(ui, "about.close.icon")
                .position(dialog_width - 52.0f, 18.0f)
                .size(34.0f, 34.0f)
                .text("×")
                .fontSize(22.0f)
                .theme(tokens, false)
                .onClick([] {
                    app_state.about_dialog_open = false;
                    controller::request_full_repaint();
                })
                .build();

            ui.text("about.description")
                .position(28.0f, 132.0f)
                .size(dialog_width - 56.0f, 54.0f)
                .text(controller::tr(i18n::Text::AboutDescription))
                .fontFamily(fonts::ui())
                .fontSize(18.0f)
                .lineHeight(25.0f)
                .wrap(true)
                .color(tokens.text)
                .build();

            ui.rect("about.privacy.background")
                .position(28.0f, 202.0f)
                .size(dialog_width - 56.0f, 86.0f)
                .color(tokens.surfaceHover)
                .radius(12.0f)
                .border(1.0f, tokens.border)
                .build();

            ui.text("about.privacy.icon")
                .position(46.0f, 222.0f)
                .size(28.0f, 40.0f)
                .text("●")
                .fontSize(18.0f)
                .verticalAlign(eui::VerticalAlign::Center)
                .color(tokens.primary)
                .build();

            ui.text("about.privacy.text")
                .position(82.0f, 216.0f)
                .size(dialog_width - 128.0f, 56.0f)
                .text(controller::tr(i18n::Text::PrivacySummary))
                .fontFamily(fonts::ui())
                .fontSize(16.0f)
                .lineHeight(23.0f)
                .wrap(true)
                .verticalAlign(eui::VerticalAlign::Center)
                .color(tokens.text)
                .build();

            components::button(ui, "about.project")
                .position(28.0f, dialog_height - 72.0f)
                .size(190.0f, 42.0f)
                .text(controller::tr(i18n::Text::ProjectWebsite))
                .icon(0xF0C1)
                .iconSize(17.0f)
                .fontSize(17.0f)
                .theme(tokens, true)
                .onClick([] { eui::platform::openUrl(kProjectUrl); })
                .build();

            ui.text("about.license")
                .position(238.0f, dialog_height - 72.0f)
                .size(dialog_width - 266.0f, 42.0f)
                .text(controller::tr(i18n::Text::License))
                .fontFamily(fonts::ui())
                .fontSize(15.0f)
                .horizontalAlign(eui::HorizontalAlign::Right)
                .verticalAlign(eui::VerticalAlign::Center)
                .color(components::theme::withAlpha(tokens.text, 0.66f))
                .build();
        })
        .build();
}

} // namespace app::views
