#include "zeus/mongo_shell_formatter.h"

#include <charconv>
#include <cctype>
#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <utility>

namespace zeus {
namespace {

struct ConstructorDefinition {
    std::string_view name;
    std::string_view extended_json_key;
};

constexpr ConstructorDefinition kConstructors[] = {
    {"NumberInt", "$numberInt"},
    {"NumberLong", "$numberLong"},
    {"NumberDecimal", "$numberDecimal"},
    {"Decimal128", "$numberDecimal"},
    {"ObjectId", "$oid"},
    {"ISODate", "$date"},
};

bool is_identifier_character(unsigned char ch) {
    return std::isalnum(ch) != 0 || ch == '_' || ch == '$';
}

void skip_space(std::string_view input, std::size_t& position) {
    while (position < input.size() &&
           std::isspace(static_cast<unsigned char>(input[position])) != 0) {
        ++position;
    }
}

ParseIssue issue_at(
    std::string_view input,
    std::size_t offset,
    std::string code,
    std::string message) {
    ParseIssue issue;
    issue.code = std::move(code);
    issue.message = std::move(message);
    issue.offset = offset;
    issue.line = 1;
    issue.column = 1;
    const std::size_t end = offset < input.size() ? offset : input.size();
    for (std::size_t index = 0; index < end; ++index) {
        if (input[index] == '\n') {
            ++issue.line;
            issue.column = 1;
        } else {
            ++issue.column;
        }
    }
    return issue;
}

bool append_utf8(std::uint32_t codepoint, std::string& output) {
    if (codepoint > 0x10FFFFU || (codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
        return false;
    }
    if (codepoint <= 0x7FU) {
        output.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FFU) {
        output.push_back(static_cast<char>(0xC0U | (codepoint >> 6U)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    } else if (codepoint <= 0xFFFFU) {
        output.push_back(static_cast<char>(0xE0U | (codepoint >> 12U)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    } else {
        output.push_back(static_cast<char>(0xF0U | (codepoint >> 18U)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    }
    return true;
}

int hex_value(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

bool read_hex_quad(std::string_view input, std::size_t& position, std::uint32_t& value) {
    if (position + 4 > input.size()) return false;
    value = 0;
    for (int index = 0; index < 4; ++index) {
        const int digit = hex_value(input[position++]);
        if (digit < 0) return false;
        value = (value << 4U) | static_cast<std::uint32_t>(digit);
    }
    return true;
}

bool read_quoted_string(
    std::string_view input,
    std::size_t& position,
    char quote,
    std::string& raw,
    std::string& decoded,
    std::size_t& error_offset) {
    if (position >= input.size() || input[position] != quote) {
        error_offset = position;
        return false;
    }
    const std::size_t start = position++;
    decoded.clear();
    while (position < input.size()) {
        const unsigned char ch = static_cast<unsigned char>(input[position++]);
        if (ch == static_cast<unsigned char>(quote)) {
            raw.assign(input.substr(start, position - start));
            return true;
        }
        if (ch < 0x20U) {
            error_offset = position - 1;
            return false;
        }
        if (ch != '\\') {
            decoded.push_back(static_cast<char>(ch));
            continue;
        }
        if (position >= input.size()) {
            error_offset = position;
            return false;
        }
        const char escaped = input[position++];
        switch (escaped) {
        case '"': decoded.push_back('"'); break;
        case '\'':
            if (quote != '\'') {
                error_offset = position - 1;
                return false;
            }
            decoded.push_back('\'');
            break;
        case '\\': decoded.push_back('\\'); break;
        case '/': decoded.push_back('/'); break;
        case 'b': decoded.push_back('\b'); break;
        case 'f': decoded.push_back('\f'); break;
        case 'n': decoded.push_back('\n'); break;
        case 'r': decoded.push_back('\r'); break;
        case 't': decoded.push_back('\t'); break;
        case 'u': {
            std::uint32_t codepoint = 0;
            if (!read_hex_quad(input, position, codepoint)) {
                error_offset = position;
                return false;
            }
            if (codepoint >= 0xD800U && codepoint <= 0xDBFFU) {
                if (position + 2 > input.size() || input[position] != '\\' ||
                    input[position + 1] != 'u') {
                    error_offset = position;
                    return false;
                }
                position += 2;
                std::uint32_t low = 0;
                if (!read_hex_quad(input, position, low) ||
                    low < 0xDC00U || low > 0xDFFFU) {
                    error_offset = position;
                    return false;
                }
                codepoint = 0x10000U + ((codepoint - 0xD800U) << 10U) +
                    (low - 0xDC00U);
            }
            if (!append_utf8(codepoint, decoded)) {
                error_offset = position;
                return false;
            }
            break;
        }
        default:
            error_offset = position - 1;
            return false;
        }
    }
    error_offset = position;
    return false;
}

bool read_json_string(
    std::string_view input,
    std::size_t& position,
    std::string& raw,
    std::string& decoded,
    std::size_t& error_offset) {
    return read_quoted_string(input, position, '"', raw, decoded, error_offset);
}

std::string quote_json_string(std::string_view value) {
    constexpr char hex[] = "0123456789abcdef";
    std::string output;
    output.reserve(value.size() + 2);
    output.push_back('"');
    for (unsigned char ch : value) {
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
                output.push_back(hex[(ch >> 4U) & 0x0FU]);
                output.push_back(hex[ch & 0x0FU]);
            } else {
                output.push_back(static_cast<char>(ch));
            }
            break;
        }
    }
    output.push_back('"');
    return output;
}

bool constructor_accepts_integer_literal(std::string_view constructor) {
    return constructor == "NumberInt" || constructor == "NumberLong";
}

bool read_constructor_argument(
    std::string_view input,
    std::size_t& position,
    std::string_view constructor,
    std::string& decoded,
    std::size_t& error_offset) {
    if (position < input.size() && (input[position] == '"' || input[position] == '\'')) {
        std::string raw;
        return read_quoted_string(
            input, position, input[position], raw, decoded, error_offset);
    }
    if (!constructor_accepts_integer_literal(constructor)) {
        error_offset = position;
        return false;
    }

    const std::size_t start = position;
    if (position < input.size() && (input[position] == '+' || input[position] == '-')) {
        ++position;
    }
    const std::size_t digits = position;
    while (position < input.size() &&
           std::isdigit(static_cast<unsigned char>(input[position])) != 0) {
        ++position;
    }
    if (position == digits) {
        error_offset = position;
        return false;
    }
    decoded.assign(input.substr(start, position - start));
    if (!decoded.empty() && decoded.front() == '+') decoded.erase(decoded.begin());
    return true;
}

template <typename Integer>
bool is_integer_in_range(std::string_view value) {
    if (value.empty()) return false;
    Integer parsed{};
    const char* first = value.data();
    const char* last = value.data() + value.size();
    const auto result = std::from_chars(first, last, parsed, 10);
    return result.ec == std::errc{} && result.ptr == last;
}

bool is_decimal128_string(std::string_view value) {
    if (value == "NaN" || value == "Infinity" || value == "-Infinity") return true;
    std::size_t position = 0;
    if (position < value.size() && (value[position] == '+' || value[position] == '-')) {
        ++position;
    }
    bool digits = false;
    while (position < value.size() && std::isdigit(static_cast<unsigned char>(value[position]))) {
        digits = true;
        ++position;
    }
    if (position < value.size() && value[position] == '.') {
        ++position;
        while (position < value.size() &&
               std::isdigit(static_cast<unsigned char>(value[position]))) {
            digits = true;
            ++position;
        }
    }
    if (!digits) return false;
    if (position < value.size() && (value[position] == 'e' || value[position] == 'E')) {
        ++position;
        if (position < value.size() && (value[position] == '+' || value[position] == '-')) {
            ++position;
        }
        const std::size_t exponent_start = position;
        while (position < value.size() &&
               std::isdigit(static_cast<unsigned char>(value[position]))) {
            ++position;
        }
        if (position == exponent_start) return false;
    }
    return position == value.size();
}

bool is_object_id(std::string_view value) {
    if (value.size() != 24) return false;
    for (char ch : value) {
        if (hex_value(ch) < 0) return false;
    }
    return true;
}

bool is_iso_date(std::string_view value) {
    if (value.size() < 20 || value[4] != '-' || value[7] != '-' ||
        (value[10] != 'T' && value[10] != 't')) {
        return false;
    }
    for (std::size_t index : {0U, 1U, 2U, 3U, 5U, 6U, 8U, 9U}) {
        if (!std::isdigit(static_cast<unsigned char>(value[index]))) return false;
    }
    return value.back() == 'Z' || value.back() == 'z' ||
        (value.size() >= 6 && (value[value.size() - 6] == '+' ||
                              value[value.size() - 6] == '-'));
}

bool valid_argument(std::string_view constructor, std::string_view value) {
    if (constructor == "NumberInt") return is_integer_in_range<std::int32_t>(value);
    if (constructor == "NumberLong") return is_integer_in_range<std::int64_t>(value);
    if (constructor == "NumberDecimal" || constructor == "Decimal128") {
        return is_decimal128_string(value);
    }
    if (constructor == "ObjectId") return is_object_id(value);
    if (constructor == "ISODate") return is_iso_date(value);
    return false;
}

const ConstructorDefinition* constructor_at(
    std::string_view input,
    std::size_t position) {
    for (const auto& definition : kConstructors) {
        if (position + definition.name.size() > input.size() ||
            input.substr(position, definition.name.size()) != definition.name) {
            continue;
        }
        const std::size_t end = position + definition.name.size();
        if (end == input.size() ||
            !is_identifier_character(static_cast<unsigned char>(input[end]))) {
            return &definition;
        }
    }
    return nullptr;
}

bool copy_json_string(
    std::string_view input,
    std::size_t& position,
    std::string& output,
    ParseIssue& issue) {
    std::string raw;
    std::string decoded;
    std::size_t error_offset = position;
    if (!read_json_string(input, position, raw, decoded, error_offset)) {
        issue = issue_at(input, error_offset, "MONGO_SHELL_STRING",
                         "Invalid string literal in MongoDB Shell input");
        return false;
    }
    output += raw;
    return true;
}

bool convert_constructor(
    std::string_view input,
    std::size_t& position,
    std::string& output,
    ParseIssue& issue) {
    const std::size_t start = position;
    std::size_t cursor = position;
    if (input.substr(cursor, 3) == "new" &&
        (cursor + 3 == input.size() ||
         !is_identifier_character(static_cast<unsigned char>(input[cursor + 3])))) {
        cursor += 3;
        const std::size_t before_space = cursor;
        skip_space(input, cursor);
        if (cursor == before_space) return false;
    }

    const ConstructorDefinition* definition = constructor_at(input, cursor);
    if (definition == nullptr) return false;
    cursor += definition->name.size();
    skip_space(input, cursor);
    if (cursor >= input.size() || input[cursor] != '(') {
        issue = issue_at(input, cursor, "MONGO_SHELL_SYNTAX",
                         "Expected '(' after MongoDB constructor");
        return false;
    }
    ++cursor;
    skip_space(input, cursor);
    std::string decoded;
    std::size_t error_offset = cursor;
    if (!read_constructor_argument(
            input, cursor, definition->name, decoded, error_offset)) {
        issue = issue_at(input, error_offset, "MONGO_SHELL_ARGUMENT",
                         "MongoDB constructor requires one literal value");
        return false;
    }
    skip_space(input, cursor);
    if (cursor >= input.size() || input[cursor] != ')') {
        issue = issue_at(input, cursor, "MONGO_SHELL_ARGUMENT",
                         "MongoDB constructor accepts one quoted value");
        return false;
    }
    ++cursor;
    if (!valid_argument(definition->name, decoded)) {
        issue = issue_at(input, start, "MONGO_SHELL_VALUE",
                         "MongoDB constructor value is invalid or out of range");
        return false;
    }

    output += "{\"";
    output += definition->extended_json_key;
    output += "\":";
    output += quote_json_string(decoded);
    output += '}';
    position = cursor;
    return true;
}

} // namespace

bool looks_like_mongo_shell(const std::string& input) {
    for (const auto& definition : kConstructors) {
        std::size_t position = input.find(definition.name);
        while (position != std::string::npos) {
            const bool left_boundary = position == 0 ||
                !is_identifier_character(static_cast<unsigned char>(input[position - 1]));
            std::size_t after = position + definition.name.size();
            skip_space(input, after);
            if (left_boundary && after < input.size() && input[after] == '(') return true;
            position = input.find(definition.name, position + 1);
        }
    }
    return false;
}

MongoShellFormatResult convert_mongo_shell_to_extended_json(
    const std::string& input,
    int indent_width) {
    MongoShellFormatResult result;
    std::string transformed;
    transformed.reserve(input.size() + 64);
    const std::string_view view(input);

    for (std::size_t position = 0; position < view.size();) {
        if (view[position] == '"') {
            if (!copy_json_string(view, position, transformed, result.json.issue)) return result;
            continue;
        }
        const std::size_t before = position;
        if (convert_constructor(view, position, transformed, result.json.issue)) {
            ++result.converted_constructors;
            continue;
        }
        if (!result.json.issue.code.empty()) return result;
        position = before;
        transformed.push_back(view[position++]);
    }

    if (result.converted_constructors == 0) {
        result.json.issue = issue_at(view, 0, "NOT_MONGO_SHELL",
                                     "No supported MongoDB constructor was found");
        return result;
    }
    result.json = format_json(transformed, indent_width);
    if (!result.json.ok && result.json.issue.code.rfind("PARSE_JSON", 0) == 0) {
        result.json.issue.code = "PARSE_MONGO_SHELL";
        result.json.issue.message = "Input contains unsupported MongoDB Shell syntax";
    }
    return result;
}

} // namespace zeus
