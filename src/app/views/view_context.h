#pragma once

#include "components/theme.h"

namespace app::views {

struct ViewContext {
    components::theme::ThemeColorTokens tokens;
    components::theme::ThemeColorTokens language_tokens;
    bool dark_theme = false;
    float screen_width = 0.0f;
    float screen_height = 0.0f;
    float margin = 0.0f;
    float content_width = 0.0f;
    float header_height = 0.0f;
    float input_height = 0.0f;
    float actions_y = 0.0f;
    float actions_height = 0.0f;
    float result_y = 0.0f;
    float result_height = 0.0f;
    float bottom_bar_y = 0.0f;
    float bottom_bar_height = 0.0f;
    float header_spacer_width = 0.0f;
    float theme_button_center_x = 0.0f;
    float about_button_center_x = 0.0f;
};

} // namespace app::views
