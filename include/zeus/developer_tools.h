#pragma once

#include "zeus/json_formatter.h"

#include <string>

namespace zeus {

std::string encode_html_entities(const std::string& input);
FormatResult decode_html_entities(const std::string& input);
std::string encode_hex(const std::string& input);
FormatResult decode_hex(const std::string& input);
bool looks_like_hex_encoding(const std::string& input);
bool looks_like_unix_timestamp(const std::string& input);
FormatResult format_unix_timestamp(const std::string& input);

} // namespace zeus
