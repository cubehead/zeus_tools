#pragma once

#include "zeus/json_formatter.h"

#include <cstddef>
#include <string>

namespace zeus {

struct MongoShellFormatResult {
    FormatResult json;
    std::size_t converted_constructors = 0;
};

// Converts a deliberately small, non-executable subset of MongoDB Shell values
// into type-preserving MongoDB Extended JSON. The result is accepted only when
// the complete transformed document is valid JSON.
MongoShellFormatResult convert_mongo_shell_to_extended_json(
    const std::string& input,
    int indent_width = 2);

// Cheap, conservative preflight used before attempting automatic conversion.
bool looks_like_mongo_shell(const std::string& input);

} // namespace zeus
