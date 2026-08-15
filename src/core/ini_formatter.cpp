#include "zeus/ini_formatter.h"

#include "zeus/json_formatter.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>

namespace zeus {
namespace {

struct Entry {
    std::string section;
    std::string key;
    std::string value;
};

struct ParsedIni {
    bool ok = true;
    std::vector<std::string> formatted_lines;
    std::vector<Entry> entries;
    ParseIssue issue;
    std::size_t assignments = 0;
    std::size_t sections = 0;
};

std::string trim(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    return first < last ? std::string(first, last) : std::string{};
}

ParsedIni parse_ini(const std::string& input) {
    ParsedIni parsed;
    std::istringstream stream(input);
    std::string section;
    std::string line;
    std::size_t line_number = 0;
    std::unordered_set<std::string> keys;
    while (std::getline(stream, line)) {
        ++line_number;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::string cleaned = trim(line);
        if (cleaned.empty()) {
            parsed.formatted_lines.emplace_back();
            continue;
        }
        if (cleaned.front() == ';' || cleaned.front() == '#' || cleaned.front() == '!') {
            parsed.formatted_lines.push_back(cleaned);
            continue;
        }
        if (cleaned.front() == '[') {
            if (cleaned.size() < 3 || cleaned.back() != ']') {
                parsed.ok = false;
                parsed.issue = {"PARSE_INI_SECTION", "INI section header must end with ']'", 0, line_number, 1};
                return parsed;
            }
            section = trim(cleaned.substr(1, cleaned.size() - 2));
            if (section.empty()) {
                parsed.ok = false;
                parsed.issue = {"PARSE_INI_SECTION", "INI section name cannot be empty", 0, line_number, 1};
                return parsed;
            }
            parsed.formatted_lines.push_back('[' + section + ']');
            ++parsed.sections;
            continue;
        }
        const std::size_t equals = cleaned.find('=');
        const std::size_t colon = cleaned.find(':');
        const std::size_t separator = equals == std::string::npos ? colon
            : colon == std::string::npos ? equals : std::min(equals, colon);
        if (separator == std::string::npos) {
            parsed.ok = false;
            parsed.issue = {"PARSE_INI_ASSIGNMENT", "Expected key=value or key:value", 0, line_number, 1};
            return parsed;
        }
        const std::string key = trim(cleaned.substr(0, separator));
        const std::string value = trim(cleaned.substr(separator + 1));
        if (key.empty()) {
            parsed.ok = false;
            parsed.issue = {"PARSE_INI_KEY", "INI key cannot be empty", 0, line_number, 1};
            return parsed;
        }
        const std::string identity = section + '\0' + key;
        if (!keys.insert(identity).second) {
            parsed.ok = false;
            parsed.issue = {"PARSE_INI_DUPLICATE", "Duplicate INI key would lose data during conversion", 0, line_number, 1};
            return parsed;
        }
        parsed.entries.push_back({section, key, value});
        parsed.formatted_lines.push_back(key + " = " + value);
        ++parsed.assignments;
    }
    if (parsed.assignments == 0) {
        parsed.ok = false;
        parsed.issue = {"PARSE_INI_EMPTY", "INI input does not contain any assignments", 0, 1, 1};
    }
    return parsed;
}

void append_json_string(std::string& output, const std::string& value) {
    constexpr char digits[] = "0123456789ABCDEF";
    output.push_back('"');
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"': output += "\\\""; break;
        case '\\': output += "\\\\"; break;
        case '\b': output += "\\b"; break;
        case '\f': output += "\\f"; break;
        case '\n': output += "\\n"; break;
        case '\r': output += "\\r"; break;
        case '\t': output += "\\t"; break;
        default:
            if (ch < 0x20U) {
                output += "\\u00";
                output.push_back(digits[ch >> 4U]);
                output.push_back(digits[ch & 0x0FU]);
            } else output.push_back(static_cast<char>(ch));
        }
    }
    output.push_back('"');
}

} // namespace

bool looks_like_ini(const std::string& input) {
    const ParsedIni parsed = parse_ini(input);
    return parsed.ok && (parsed.assignments >= 2 ||
        (parsed.assignments >= 1 && parsed.sections >= 1));
}

FormatResult format_ini(const std::string& input) {
    const ParsedIni parsed = parse_ini(input);
    if (!parsed.ok) return {false, {}, parsed.issue};
    std::string output;
    for (std::size_t index = 0; index < parsed.formatted_lines.size(); ++index) {
        if (index != 0) output.push_back('\n');
        output += parsed.formatted_lines[index];
    }
    return {true, std::move(output), {}};
}

FormatResult ini_to_json(const std::string& input, int indent_width) {
    const ParsedIni parsed = parse_ini(input);
    if (!parsed.ok) return {false, {}, parsed.issue};
    std::vector<std::string> section_order;
    std::map<std::string, std::vector<Entry>> sections;
    for (const auto& entry : parsed.entries) {
        if (sections.find(entry.section) == sections.end()) section_order.push_back(entry.section);
        sections[entry.section].push_back(entry);
    }
    std::string compact = "{";
    bool first = true;
    const auto append_pair = [&](const std::string& key, const std::string& value,
                                 std::string& target, bool& first_pair) {
        if (!first_pair) target.push_back(',');
        append_json_string(target, key);
        target.push_back(':');
        append_json_string(target, value);
        first_pair = false;
    };
    const auto root = sections.find("");
    if (root != sections.end()) {
        for (const auto& entry : root->second) append_pair(entry.key, entry.value, compact, first);
    }
    for (const auto& section : section_order) {
        if (section.empty()) continue;
        if (!first) compact.push_back(',');
        append_json_string(compact, section);
        compact += ":{";
        bool first_entry = true;
        for (const auto& entry : sections[section]) {
            append_pair(entry.key, entry.value, compact, first_entry);
        }
        compact.push_back('}');
        first = false;
    }
    compact.push_back('}');
    return format_json(compact, indent_width);
}

} // namespace zeus
