#pragma once

#include "zeus/json_formatter.h"

#include <string>

namespace zeus {

FormatResult format_xml(const std::string& input, int indent_width = 2);

} // namespace zeus
