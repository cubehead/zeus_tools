#pragma once

#include "zeus/json_formatter.h"

#include <string>

namespace zeus {

FormatResult json_to_yaml(const std::string& input, int indent_width = 2);
FormatResult json_to_xml(const std::string& input, int indent_width = 2);
FormatResult xml_to_json(const std::string& input, int indent_width = 2);
FormatResult json_to_csv(const std::string& input);
FormatResult yaml_to_json(const std::string& input, int indent_width = 2);
FormatResult unescape_json_string(const std::string& input);

} // namespace zeus
