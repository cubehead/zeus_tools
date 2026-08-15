#pragma once

#include "zeus/json_formatter.h"

#include <string>

namespace zeus {

bool looks_like_ini(const std::string& input);
FormatResult format_ini(const std::string& input);
FormatResult ini_to_json(const std::string& input, int indent_width = 2);

} // namespace zeus
