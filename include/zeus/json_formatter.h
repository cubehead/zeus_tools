#pragma once

#include <cstddef>
#include <string>

namespace zeus {

struct ParseIssue {
    std::string code;
    std::string message;
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct FormatResult {
    bool ok = false;
    std::string value;
    ParseIssue issue;
};

FormatResult format_json(const std::string& input, int indent_width = 2);

} // namespace zeus
