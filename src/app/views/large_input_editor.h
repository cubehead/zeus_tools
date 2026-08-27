#pragma once

#include "view_context.h"

#include "eui/dsl.h"

namespace app::views {

void build_large_input_editor(eui::Ui& ui, const ViewContext& context);
bool apply_large_input_history(bool redo);

} // namespace app::views
