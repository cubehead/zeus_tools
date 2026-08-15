#pragma once

#include "zeus/json_formatter.h"

#include <string>

namespace zeus {

bool looks_like_toml(const std::string& input);
FormatResult format_toml(const std::string& input);
FormatResult toml_to_json(const std::string& input, int indent_width = 2);
FormatResult json_to_toml(const std::string& input);

} // namespace zeus
