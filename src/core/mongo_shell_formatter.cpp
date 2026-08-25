#include "zeus/mongo_shell_formatter.h"

#include <algorithm>
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
    {"Int32", "$numberInt"},
    {"NumberLong", "$numberLong"},
    {"Double", "$numberDouble"},
    {"NumberDecimal", "$numberDecimal"},
    {"Decimal128", "$numberDecimal"},
    {"ObjectId", "$oid"},
    {"ISODate", "$date"},
    {"Date", "$date"},
    {"UUID", "$uuid"},
    {"Timestamp", "$timestamp"},
    {"BinData", "$binary"},
    {"BSONRegExp", "$regularExpression"},
    {"Code", "$code"},
    {"MinKey", "$minKey"},
    {"MaxKey", "$maxKey"},
};

bool is_identifier_character(unsigned char ch) {
    return std::isalnum(ch) != 0 || ch == '_' || ch == '$';
}

bool is_identifier_start(unsigned char ch) {
    return std::isalpha(ch) != 0 || ch == '_' || ch == '$';
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

bool constructor_accepts_number_literal(std::string_view constructor) {
    return constructor == "NumberInt" || constructor == "Int32" ||
        constructor == "NumberLong" || constructor == "Double" || constructor == "Date";
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
    if (!constructor_accepts_number_literal(constructor)) {
        error_offset = position;
        return false;
    }

    const std::size_t start = position;
    if (position < input.size() && (input[position] == '+' || input[position] == '-')) {
        ++position;
    }
    const std::size_t value_start = position;
    if (constructor == "Double" && position < input.size() &&
        std::isalpha(static_cast<unsigned char>(input[position])) != 0) {
        while (position < input.size() &&
               std::isalpha(static_cast<unsigned char>(input[position])) != 0) {
            ++position;
        }
    } else {
        while (position < input.size()) {
            const char ch = input[position];
            if (std::isdigit(static_cast<unsigned char>(ch)) == 0 &&
                ch != '.' && ch != 'e' && ch != 'E' && ch != '+' && ch != '-') {
                break;
            }
            ++position;
        }
    }
    if (position == value_start) {
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

template <typename Integer>
std::string canonical_integer_string(std::string_view value) {
    Integer parsed{};
    const char* first = value.data();
    const char* last = value.data() + value.size();
    const auto result = std::from_chars(first, last, parsed, 10);
    if (result.ec != std::errc{} || result.ptr != last) return {};
    return std::to_string(parsed);
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

bool is_uuid(std::string_view value) {
    if (value.size() != 36) return false;
    for (std::size_t index = 0; index < value.size(); ++index) {
        const bool separator = index == 8 || index == 13 || index == 18 || index == 23;
        if (separator ? value[index] != '-' : hex_value(value[index]) < 0) return false;
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
    if (constructor == "NumberInt" || constructor == "Int32") {
        return is_integer_in_range<std::int32_t>(value);
    }
    if (constructor == "NumberLong") return is_integer_in_range<std::int64_t>(value);
    if (constructor == "Double") return is_decimal128_string(value);
    if (constructor == "NumberDecimal" || constructor == "Decimal128") {
        return is_decimal128_string(value);
    }
    if (constructor == "ObjectId") return is_object_id(value);
    if (constructor == "ISODate") return is_iso_date(value);
    if (constructor == "Date") {
        return is_iso_date(value) || is_integer_in_range<std::int64_t>(value);
    }
    if (constructor == "UUID") return is_uuid(value);
    if (constructor == "Code") return true;
    return false;
}

bool read_uint32_literal(
    std::string_view input,
    std::size_t& position,
    std::uint32_t& value) {
    const std::size_t start = position;
    while (position < input.size() &&
           std::isdigit(static_cast<unsigned char>(input[position])) != 0) {
        ++position;
    }
    if (position == start) return false;
    const char* first = input.data() + start;
    const char* last = input.data() + position;
    const auto parsed = std::from_chars(first, last, value, 10);
    return parsed.ec == std::errc{} && parsed.ptr == last;
}

bool read_timestamp_field(
    std::string_view input,
    std::size_t& position,
    char& field) {
    if (position < input.size() && (input[position] == '"' || input[position] == '\'')) {
        std::string raw;
        std::string decoded;
        std::size_t error_offset = position;
        if (!read_quoted_string(
                input, position, input[position], raw, decoded, error_offset) ||
            decoded.size() != 1 || (decoded[0] != 't' && decoded[0] != 'i')) {
            return false;
        }
        field = decoded[0];
        return true;
    }
    if (position < input.size() && (input[position] == 't' || input[position] == 'i')) {
        field = input[position++];
        return position == input.size() ||
            !is_identifier_character(static_cast<unsigned char>(input[position]));
    }
    return false;
}

bool read_timestamp_arguments(
    std::string_view input,
    std::size_t& position,
    std::uint32_t& seconds,
    std::uint32_t& increment) {
    skip_space(input, position);
    if (position >= input.size()) return false;
    if (input[position] != '{') {
        if (!read_uint32_literal(input, position, seconds)) return false;
        skip_space(input, position);
        if (position >= input.size() || input[position++] != ',') return false;
        skip_space(input, position);
        return read_uint32_literal(input, position, increment);
    }

    ++position;
    bool have_seconds = false;
    bool have_increment = false;
    for (int member = 0; member < 2; ++member) {
        skip_space(input, position);
        char field = '\0';
        if (!read_timestamp_field(input, position, field)) return false;
        skip_space(input, position);
        if (position >= input.size() || input[position++] != ':') return false;
        skip_space(input, position);
        std::uint32_t value = 0;
        if (!read_uint32_literal(input, position, value)) return false;
        if (field == 't') {
            if (have_seconds) return false;
            seconds = value;
            have_seconds = true;
        } else {
            if (have_increment) return false;
            increment = value;
            have_increment = true;
        }
        skip_space(input, position);
        if (member == 0) {
            if (position >= input.size() || input[position++] != ',') return false;
        }
    }
    skip_space(input, position);
    if (position >= input.size() || input[position++] != '}') return false;
    return have_seconds && have_increment;
}

int base64_value(char ch) {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
    if (ch >= '0' && ch <= '9') return ch - '0' + 52;
    if (ch == '+') return 62;
    if (ch == '/') return 63;
    return -1;
}

bool is_canonical_base64(std::string_view value) {
    if (value.empty()) return true;
    if (value.size() % 4 != 0) return false;
    std::size_t padding = 0;
    if (value.back() == '=') ++padding;
    if (value.size() > 1 && value[value.size() - 2] == '=') ++padding;
    for (std::size_t index = 0; index < value.size() - padding; ++index) {
        if (base64_value(value[index]) < 0) return false;
    }
    for (std::size_t index = value.size() - padding; index < value.size(); ++index) {
        if (value[index] != '=') return false;
    }
    if (padding > 2 || value.size() - padding < 2) return false;
    if (padding == 2) {
        return (base64_value(value[value.size() - 3]) & 0x0F) == 0;
    }
    if (padding == 1) {
        return (base64_value(value[value.size() - 2]) & 0x03) == 0;
    }
    return true;
}

bool read_bin_data_arguments(
    std::string_view input,
    std::size_t& position,
    std::uint32_t& subtype,
    std::string& payload) {
    skip_space(input, position);
    if (!read_uint32_literal(input, position, subtype) || subtype > 255U) return false;
    skip_space(input, position);
    if (position >= input.size() || input[position++] != ',') return false;
    skip_space(input, position);
    if (position >= input.size() || (input[position] != '"' && input[position] != '\'')) {
        return false;
    }
    std::string raw;
    std::size_t error_offset = position;
    if (!read_quoted_string(
            input, position, input[position], raw, payload, error_offset)) {
        return false;
    }
    return is_canonical_base64(payload);
}

bool normalize_bson_regex_options(std::string& options) {
    for (std::size_t index = 0; index < options.size(); ++index) {
        const char option = options[index];
        if (option != 'i' && option != 'l' && option != 'm' &&
            option != 's' && option != 'u' && option != 'x') {
            return false;
        }
        if (options.find(option, index + 1) != std::string::npos) return false;
    }
    std::sort(options.begin(), options.end());
    return true;
}

bool read_bson_regex_arguments(
    std::string_view input,
    std::size_t& position,
    std::string& pattern,
    std::string& options) {
    skip_space(input, position);
    if (position >= input.size() || (input[position] != '"' && input[position] != '\'')) {
        return false;
    }
    std::string raw;
    std::size_t error_offset = position;
    if (!read_quoted_string(
            input, position, input[position], raw, pattern, error_offset)) {
        return false;
    }
    skip_space(input, position);
    if (position >= input.size() || input[position++] != ',') return false;
    skip_space(input, position);
    if (position >= input.size() || (input[position] != '"' && input[position] != '\'')) {
        return false;
    }
    if (!read_quoted_string(
            input, position, input[position], raw, options, error_offset)) {
        return false;
    }
    return normalize_bson_regex_options(options);
}

bool convert_regex_literal(
    std::string_view input,
    std::size_t& position,
    std::string& output,
    ParseIssue& issue) {
    if (position >= input.size() || input[position] != '/') return false;
    const std::size_t start = position++;
    std::string pattern;
    bool closed = false;
    while (position < input.size()) {
        const unsigned char ch = static_cast<unsigned char>(input[position++]);
        if (ch == '/') {
            closed = true;
            break;
        }
        if (ch == '\n' || ch == '\r' || ch < 0x20U) {
            issue = issue_at(input, position - 1, "MONGO_SHELL_REGEX",
                             "MongoDB regex literals cannot contain raw control characters");
            return false;
        }
        if (ch != '\\') {
            pattern.push_back(static_cast<char>(ch));
            continue;
        }
        if (position >= input.size() || input[position] == '\n' || input[position] == '\r') {
            issue = issue_at(input, position, "MONGO_SHELL_REGEX",
                             "MongoDB regex literal has an incomplete escape");
            return false;
        }
        const char escaped = input[position++];
        if (escaped == '/') {
            pattern.push_back('/');
        } else {
            pattern.push_back('\\');
            pattern.push_back(escaped);
        }
    }
    if (!closed) {
        issue = issue_at(input, start, "MONGO_SHELL_REGEX",
                         "MongoDB regex literal is missing its closing slash");
        return false;
    }

    std::string options;
    while (position < input.size() &&
           std::isalpha(static_cast<unsigned char>(input[position])) != 0) {
        const char option = input[position++];
        if (option != 'i' && option != 'm' && option != 's' &&
            option != 'u' && option != 'x') {
            issue = issue_at(input, position - 1, "MONGO_SHELL_REGEX_OPTION",
                             "MongoDB regex option is not supported");
            return false;
        }
        if (options.find(option) != std::string::npos) {
            issue = issue_at(input, position - 1, "MONGO_SHELL_REGEX_OPTION",
                             "MongoDB regex options must not repeat");
            return false;
        }
        options.push_back(option);
    }
    std::sort(options.begin(), options.end());
    output += "{\"$regularExpression\":{\"pattern\":";
    output += quote_json_string(pattern);
    output += ",\"options\":";
    output += quote_json_string(options);
    output += "}}";
    return true;
}

bool has_regex_literal_position(std::string_view input) {
    char quote = '\0';
    bool escaped = false;
    char previous = '\0';
    for (char ch : input) {
        if (quote != '\0') {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == quote) {
                quote = '\0';
            }
            continue;
        }
        if (ch == '"' || ch == '\'') {
            quote = ch;
            continue;
        }
        if (ch == '/' && (previous == '\0' || previous == ':' ||
                          previous == '[' || previous == ',')) {
            return true;
        }
        if (std::isspace(static_cast<unsigned char>(ch)) == 0) previous = ch;
    }
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

bool copy_shell_string(
    std::string_view input,
    std::size_t& position,
    std::string& output,
    ParseIssue& issue) {
    std::string raw;
    std::string decoded;
    std::size_t error_offset = position;
    if (!read_quoted_string(
            input, position, '\'', raw, decoded, error_offset)) {
        issue = issue_at(input, error_offset, "MONGO_SHELL_STRING",
                         "Invalid single-quoted MongoDB Shell string");
        return false;
    }
    output += quote_json_string(decoded);
    return true;
}

bool convert_unquoted_key(
    std::string_view input,
    std::size_t& position,
    std::string& output) {
    if (position >= input.size() ||
        !is_identifier_start(static_cast<unsigned char>(input[position]))) {
        return false;
    }
    std::size_t end = position + 1;
    while (end < input.size() &&
           is_identifier_character(static_cast<unsigned char>(input[end]))) {
        ++end;
    }
    std::size_t colon = end;
    skip_space(input, colon);
    if (colon >= input.size() || input[colon] != ':') return false;
    output += quote_json_string(input.substr(position, end - position));
    position = end;
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
    if (definition->name == "Timestamp") {
        std::uint32_t seconds = 0;
        std::uint32_t increment = 0;
        if (!read_timestamp_arguments(input, cursor, seconds, increment)) {
            issue = issue_at(input, cursor, "MONGO_SHELL_ARGUMENT",
                             "Timestamp requires unsigned t and i values");
            return false;
        }
        skip_space(input, cursor);
        if (cursor >= input.size() || input[cursor] != ')') {
            issue = issue_at(input, cursor, "MONGO_SHELL_ARGUMENT",
                             "Timestamp accepts only t and i values");
            return false;
        }
        ++cursor;
        output += "{\"$timestamp\":{\"t\":";
        output += std::to_string(seconds);
        output += ",\"i\":";
        output += std::to_string(increment);
        output += "}}";
        position = cursor;
        return true;
    }
    if (definition->name == "BinData") {
        std::uint32_t subtype = 0;
        std::string payload;
        if (!read_bin_data_arguments(input, cursor, subtype, payload)) {
            issue = issue_at(input, cursor, "MONGO_SHELL_ARGUMENT",
                             "BinData requires a byte subtype and canonical Base64 payload");
            return false;
        }
        skip_space(input, cursor);
        if (cursor >= input.size() || input[cursor] != ')') {
            issue = issue_at(input, cursor, "MONGO_SHELL_ARGUMENT",
                             "BinData accepts only subtype and Base64 values");
            return false;
        }
        ++cursor;
        constexpr char hex[] = "0123456789abcdef";
        output += "{\"$binary\":{\"base64\":";
        output += quote_json_string(payload);
        output += ",\"subType\":\"";
        output.push_back(hex[(subtype >> 4U) & 0x0FU]);
        output.push_back(hex[subtype & 0x0FU]);
        output += "\"}}";
        position = cursor;
        return true;
    }
    if (definition->name == "BSONRegExp") {
        std::string pattern;
        std::string options;
        if (!read_bson_regex_arguments(input, cursor, pattern, options)) {
            issue = issue_at(input, cursor, "MONGO_SHELL_REGEX",
                             "BSONRegExp requires a pattern and valid flags");
            return false;
        }
        skip_space(input, cursor);
        if (cursor >= input.size() || input[cursor] != ')') {
            issue = issue_at(input, cursor, "MONGO_SHELL_ARGUMENT",
                             "BSONRegExp accepts only pattern and flags");
            return false;
        }
        ++cursor;
        output += "{\"$regularExpression\":{\"pattern\":";
        output += quote_json_string(pattern);
        output += ",\"options\":";
        output += quote_json_string(options);
        output += "}}";
        position = cursor;
        return true;
    }
    if (definition->name == "MinKey" || definition->name == "MaxKey") {
        skip_space(input, cursor);
        if (cursor >= input.size() || input[cursor] != ')') {
            issue = issue_at(input, cursor, "MONGO_SHELL_ARGUMENT",
                             "MinKey and MaxKey do not accept arguments");
            return false;
        }
        ++cursor;
        output += definition->name == "MinKey"
            ? "{\"$minKey\":1}" : "{\"$maxKey\":1}";
        position = cursor;
        return true;
    }
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
                         "MongoDB constructor accepts one literal value");
        return false;
    }
    ++cursor;
    if (!valid_argument(definition->name, decoded)) {
        issue = issue_at(input, start, "MONGO_SHELL_VALUE",
                         "MongoDB constructor value is invalid or out of range");
        return false;
    }

    if (definition->name == "Date") {
        output += "{\"$date\":";
        if (is_integer_in_range<std::int64_t>(decoded)) {
            output += "{\"$numberLong\":";
            output += quote_json_string(canonical_integer_string<std::int64_t>(decoded));
            output += '}';
        } else {
            output += quote_json_string(decoded);
        }
        output += '}';
        position = cursor;
        return true;
    }

    output += "{\"";
    output += definition->extended_json_key;
    output += "\":";
    if (definition->name == "NumberInt" || definition->name == "Int32") {
        output += quote_json_string(canonical_integer_string<std::int32_t>(decoded));
    } else if (definition->name == "NumberLong") {
        output += quote_json_string(canonical_integer_string<std::int64_t>(decoded));
    } else {
        output += quote_json_string(decoded);
    }
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
    return has_regex_literal_position(input);
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
        if (view[position] == '\'') {
            if (!copy_shell_string(view, position, transformed, result.json.issue)) return result;
            continue;
        }
        if (convert_unquoted_key(view, position, transformed)) continue;
        if (view[position] == '/') {
            if (convert_regex_literal(view, position, transformed, result.json.issue)) {
                ++result.converted_constructors;
                continue;
            }
            if (!result.json.issue.code.empty()) return result;
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
