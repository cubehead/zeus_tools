#pragma once

#include "eui/dsl.h"

namespace app::accessibility {

// Publish the current canvas semantics to the native accessibility bridge.
void publish(eui::Ui& ui, const eui::Screen& screen);

} // namespace app::accessibility
