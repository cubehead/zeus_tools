#include "zeus/yaml_formatter.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace zeus {
namespace {

constexpr std::size_t max_yaml_bytes = 10U * 1024U * 1024U;
constexpr std::size_t max_yaml_nodes = 100000U;
constexpr std::size_t max_yaml_depth = 128U;

std::string trim_ascii(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    return first < last ? std::string(first, last) : std::string{};
}

ParseIssue issue(const char* code, const std::string& message,
                 std::size_t line = 1, std::size_t column = 1) {
    ParseIssue value;
    value.code = code;
    value.message = message;
    value.line = line;
    value.column = column;
    return value;
}

bool unsafe_yaml_token(const std::string& input, ParseIssue& found) {
    bool single_quote = false;
    bool double_quote = false;
    bool escaped = false;
    bool token_boundary = true;
    std::size_t line = 1;
    std::size_t column = 1;
    for (std::size_t i = 0; i < input.size(); ++i) {
        const char ch = input[i];
        if (!single_quote && !double_quote && ch == '#') {
            while (i < input.size() && input[i] != '\n') {
                ++i;
                ++column;
            }
            if (i == input.size()) break;
            ++line;
            column = 1;
            token_boundary = true;
            continue;
        }
        if (ch == '\n') {
            single_quote = double_quote = escaped = false;
            token_boundary = true;
            ++line;
            column = 1;
            continue;
        }
        if (double_quote) {
            if (escaped) escaped = false;
            else if (ch == '\\') escaped = true;
            else if (ch == '"') double_quote = false;
        } else if (single_quote) {
            if (ch == '\'' && i + 1 < input.size() && input[i + 1] == '\'') {
                ++i;
                ++column;
            } else if (ch == '\'') single_quote = false;
        } else if (ch == '"') {
            double_quote = true;
        } else if (ch == '\'') {
            single_quote = true;
        } else if (token_boundary && (ch == '&' || ch == '*' || ch == '!' || ch == '%')) {
            found = issue("YAML_UNSAFE_FEATURE",
                "YAML directives, custom tags, anchors and aliases are disabled", line, column);
            return true;
        }
        token_boundary = std::isspace(static_cast<unsigned char>(ch)) || ch == ':' || ch == '-' ||
                         ch == '[' || ch == '{' || ch == ',';
        ++column;
    }
    return false;
}

bool validate_tree(const YAML::Node& node, std::size_t depth, std::size_t& nodes) {
    if (depth > max_yaml_depth || ++nodes > max_yaml_nodes) return false;
    if (node.IsSequence()) {
        for (const auto& child : node) {
            if (!validate_tree(child, depth + 1, nodes)) return false;
        }
    } else if (node.IsMap()) {
        for (const auto& entry : node) {
            if (!validate_tree(entry.first, depth + 1, nodes) ||
                !validate_tree(entry.second, depth + 1, nodes)) return false;
        }
    }
    return true;
}

} // namespace

bool looks_like_yaml(const std::string& input) {
    if (input.find('\n') == std::string::npos) return false;
    std::size_t meaningful = 0;
    std::size_t mappings = 0;
    std::size_t sequences = 0;
    std::size_t start = 0;
    while (start <= input.size()) {
        const auto end = input.find('\n', start);
        const std::string line = trim_ascii(input.substr(
            start, end == std::string::npos ? std::string::npos : end - start));
        if (!line.empty() && line.front() != '#') {
            ++meaningful;
            if (line.rfind("- ", 0) == 0) ++sequences;
            const auto colon = line.find(':');
            if (colon != std::string::npos && colon > 0 &&
                (colon + 1 == line.size() || std::isspace(static_cast<unsigned char>(line[colon + 1])))) {
                ++mappings;
            }
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return meaningful >= 2 && (mappings >= 2 || sequences >= 2 || (mappings >= 1 && sequences >= 1));
}

FormatResult format_yaml(const std::string& input, int indent_width) {
    if (input.size() > max_yaml_bytes) {
        return {false, {}, issue("YAML_TOO_LARGE", "YAML input exceeds the 10 MB safety limit")};
    }
    ParseIssue unsafe;
    if (unsafe_yaml_token(input, unsafe)) return {false, {}, std::move(unsafe)};

    try {
        const auto documents = YAML::LoadAll(input);
        if (documents.size() != 1) {
            return {false, {}, issue("YAML_MULTIPLE_DOCUMENTS",
                "Only one YAML document is supported")};
        }
        const YAML::Node& root = documents.front();
        if (!root.IsMap() && !root.IsSequence()) {
            return {false, {}, issue("YAML_SCALAR_ROOT",
                "YAML root must be a mapping or sequence")};
        }
        std::size_t nodes = 0;
        if (!validate_tree(root, 0, nodes)) {
            return {false, {}, issue("YAML_COMPLEXITY_LIMIT",
                "YAML exceeds the nesting or node-count safety limit")};
        }
        YAML::Emitter emitter;
        emitter.SetIndent(static_cast<std::size_t>(std::max(1, indent_width)));
        emitter << root;
        if (!emitter.good()) {
            return {false, {}, issue("YAML_EMIT_ERROR", emitter.GetLastError())};
        }
        FormatResult result;
        result.ok = true;
        result.value = emitter.c_str();
        return result;
    } catch (const YAML::Exception& exception) {
        const std::size_t line = exception.mark.line < 0 ? 1 : static_cast<std::size_t>(exception.mark.line) + 1;
        const std::size_t column = exception.mark.column < 0 ? 1 : static_cast<std::size_t>(exception.mark.column) + 1;
        return {false, {}, issue("INVALID_YAML", exception.msg, line, column)};
    }
}

} // namespace zeus
