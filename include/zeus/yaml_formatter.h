#pragma once

#include "zeus/json_formatter.h"

#include <string>

namespace zeus {

FormatResult format_yaml(const std::string& input, int indent_width = 2);
bool looks_like_yaml(const std::string& input);

} // namespace zeus
