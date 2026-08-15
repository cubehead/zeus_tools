#include "zeus/toml_formatter.h"

#include "zeus/json_formatter.h"

#include <toml++/toml.hpp>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>
#include <vector>

namespace zeus {
namespace {

ParseIssue issue(const char* code, const std::string& message) {
    ParseIssue result;
    result.code = code;
    result.message = message;
    return result;
}

ParseIssue toml_issue(const toml::parse_error& error) {
    ParseIssue result = issue("PARSE_TOML", std::string(error.description()));
    result.line = error.source().begin.line;
    result.column = error.source().begin.column;
    return result;
}

std::string trim_ascii(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    return first < last ? std::string(first, last) : std::string{};
}

bool bare_key(const std::string& key) {
    return !key.empty() && std::all_of(key.begin(), key.end(), [](unsigned char ch) {
        return std::isalnum(ch) != 0 || ch == '_' || ch == '-';
    });
}

bool json_number(const std::string& value) {
    std::size_t index = 0;
    if (index < value.size() && value[index] == '-') ++index;
    if (index == value.size()) return false;
    if (value[index] == '0') {
        ++index;
        if (index < value.size() && std::isdigit(static_cast<unsigned char>(value[index]))) return false;
    } else if (value[index] >= '1' && value[index] <= '9') {
        while (index < value.size() &&
               std::isdigit(static_cast<unsigned char>(value[index]))) ++index;
    } else {
        return false;
    }
    if (index < value.size() && value[index] == '.') {
        ++index;
        const std::size_t start = index;
        while (index < value.size() &&
               std::isdigit(static_cast<unsigned char>(value[index]))) ++index;
        if (index == start) return false;
    }
    if (index < value.size() && (value[index] == 'e' || value[index] == 'E')) {
        ++index;
        if (index < value.size() && (value[index] == '+' || value[index] == '-')) ++index;
        const std::size_t start = index;
        while (index < value.size() &&
               std::isdigit(static_cast<unsigned char>(value[index]))) ++index;
        if (index == start) return false;
    }
    return index == value.size();
}

std::string quote(const std::string& value) {
    std::string output = "\"";
    for (const unsigned char ch : value) {
        switch (ch) {
        case '\\': output += "\\\\"; break;
        case '"': output += "\\\""; break;
        case '\b': output += "\\b"; break;
        case '\t': output += "\\t"; break;
        case '\n': output += "\\n"; break;
        case '\f': output += "\\f"; break;
        case '\r': output += "\\r"; break;
        default:
            if (ch < 0x20U) {
                constexpr char digits[] = "0123456789ABCDEF";
                output += "\\u00";
                output.push_back(digits[ch >> 4U]);
                output.push_back(digits[ch & 0x0FU]);
            } else {
                output.push_back(static_cast<char>(ch));
            }
        }
    }
    output.push_back('"');
    return output;
}

std::string key_component(const std::string& key) {
    return bare_key(key) ? key : quote(key);
}

bool node_is_object_array(const YAML::Node& node) {
    return node.IsSequence() && node.size() != 0 &&
        std::all_of(node.begin(), node.end(), [](const YAML::Node& child) {
            return child.IsMap();
        });
}

bool emit_value(const YAML::Node& node, std::string& output, std::string& error) {
    if (!node || node.IsNull()) {
        error = "TOML does not support JSON null values";
        return false;
    }
    if (node.IsMap()) {
        error = "Nested JSON objects must be emitted as TOML tables";
        return false;
    }
    if (node.IsSequence()) {
        if (node_is_object_array(node)) {
            error = "Arrays of JSON objects must be emitted as TOML table arrays";
            return false;
        }
        output += '[';
        for (std::size_t index = 0; index < node.size(); ++index) {
            if (index != 0) output += ", ";
            if (!emit_value(node[index], output, error)) return false;
        }
        output += ']';
        return true;
    }

    const std::string value = node.Scalar();
    const std::string tag = node.Tag();
    const bool forced_string = tag == "!" || tag == "tag:yaml.org,2002:str";
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (!forced_string && (lowered == "true" || lowered == "false" || json_number(value))) {
        output += value;
    } else {
        output += quote(value);
    }
    return true;
}

std::string join_path(const std::vector<std::string>& path) {
    std::string output;
    for (std::size_t index = 0; index < path.size(); ++index) {
        if (index != 0) output.push_back('.');
        output += key_component(path[index]);
    }
    return output;
}

bool emit_table(const YAML::Node& table, std::vector<std::string> path,
                std::string& output, std::string& error, bool emit_header) {
    if (emit_header) output += '[' + join_path(path) + "]\n";

    for (const auto& entry : table) {
        if (!entry.first.IsScalar()) {
            error = "JSON object keys must be strings";
            return false;
        }
        const YAML::Node value = entry.second;
        if (value.IsMap() || node_is_object_array(value)) continue;
        output += key_component(entry.first.Scalar()) + " = ";
        if (!emit_value(value, output, error)) return false;
        output.push_back('\n');
    }

    for (const auto& entry : table) {
        const std::string key = entry.first.Scalar();
        const YAML::Node value = entry.second;
        if (value.IsMap()) {
            if (!output.empty() && output.back() != '\n') output.push_back('\n');
            if (!output.empty() && output.size() >= 2 && output[output.size() - 2] != '\n') {
                output.push_back('\n');
            }
            auto child_path = path;
            child_path.push_back(key);
            if (!emit_table(value, std::move(child_path), output, error, true)) return false;
        } else if (node_is_object_array(value)) {
            auto child_path = path;
            child_path.push_back(key);
            for (const auto& child : value) {
                if (!output.empty() && output.back() != '\n') output.push_back('\n');
                if (!output.empty() && output.size() >= 2 && output[output.size() - 2] != '\n') {
                    output.push_back('\n');
                }
                output += "[[" + join_path(child_path) + "]]\n";
                if (!emit_table(child, child_path, output, error, false)) return false;
            }
        }
    }
    return true;
}

} // namespace

bool looks_like_toml(const std::string& input) {
    const std::string trimmed = trim_ascii(input);
    if (trimmed.empty() || trimmed.find('=') == std::string::npos) return false;
    if (trimmed.find('\n') == std::string::npos && trimmed.front() != '[') return false;
    try {
        const toml::table parsed = toml::parse(input);
        return !parsed.empty();
    } catch (const toml::parse_error&) {
        return false;
    }
}

FormatResult format_toml(const std::string& input) {
    try {
        const toml::table parsed = toml::parse(input);
        std::ostringstream output;
        output << toml::toml_formatter{parsed};
        return {true, output.str(), {}};
    } catch (const toml::parse_error& error) {
        return {false, {}, toml_issue(error)};
    }
}

FormatResult toml_to_json(const std::string& input, int indent_width) {
    try {
        const toml::table parsed = toml::parse(input);
        std::ostringstream json;
        json << toml::json_formatter{parsed};
        return format_json(json.str(), indent_width);
    } catch (const toml::parse_error& error) {
        return {false, {}, toml_issue(error)};
    }
}

FormatResult json_to_toml(const std::string& input) {
    const auto valid = format_json(input, 2);
    if (!valid.ok) return valid;
    try {
        const YAML::Node root = YAML::Load(input);
        if (!root.IsMap()) {
            return {false, {}, issue("JSON_TO_TOML_ROOT", "JSON to TOML requires an object at the root")};
        }
        std::string output;
        std::string error;
        if (!emit_table(root, {}, output, error, false)) {
            return {false, {}, issue("JSON_TO_TOML_VALUE", error)};
        }
        const auto normalized = format_toml(output);
        if (!normalized.ok) {
            return {false, {}, issue("JSON_TO_TOML_VALUE", normalized.issue.message)};
        }
        return normalized;
    } catch (const YAML::Exception& error) {
        return {false, {}, issue("JSON_TO_TOML_PARSE", error.what())};
    }
}

} // namespace zeus
