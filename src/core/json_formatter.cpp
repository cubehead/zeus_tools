#include "zeus/json_formatter.h"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace zeus {
namespace {

class JsonFormatter {
public:
    JsonFormatter(const std::string& input, int indent_width)
        : input_(input), indent_width_(std::clamp(indent_width, 1, 8)) {}

    FormatResult run() {
        skip_space();
        if (!parse_value(0)) {
            return failure();
        }
        skip_space();
        if (position_ != input_.size()) {
            set_error("PARSE_JSON_TRAILING", "Unexpected content after the JSON value");
            return failure();
        }
        return {true, std::move(output_), {}};
    }

private:
    bool parse_value(int depth) {
        if (depth > 512) {
            return set_error("PARSE_JSON_DEPTH", "JSON nesting exceeds the supported depth of 512");
        }
        skip_space();
        if (position_ >= input_.size()) {
            return set_error("PARSE_JSON_VALUE", "Expected a JSON value");
        }

        switch (input_[position_]) {
        case '{': return parse_object(depth);
        case '[': return parse_array(depth);
        case '"': return parse_string();
        case 't': return parse_literal("true");
        case 'f': return parse_literal("false");
        case 'n': return parse_literal("null");
        default:
            if (input_[position_] == '-' || is_digit(input_[position_])) {
                return parse_number();
            }
            return set_error("PARSE_JSON_VALUE", "Expected an object, array, string, number, boolean, or null");
        }
    }

    bool parse_object(int depth) {
        output_.push_back('{');
        ++position_;
        skip_space();
        if (consume('}')) {
            output_.push_back('}');
            return true;
        }

        output_.push_back('\n');
        while (true) {
            append_indent(depth + 1);
            if (position_ >= input_.size() || input_[position_] != '"') {
                return set_error("PARSE_JSON_KEY", "Expected a quoted object key");
            }
            if (!parse_string()) {
                return false;
            }
            skip_space();
            if (!consume(':')) {
                return set_error("PARSE_JSON_COLON", "Expected ':' after object key");
            }
            output_ += ": ";
            if (!parse_value(depth + 1)) {
                return false;
            }
            skip_space();
            if (consume(',')) {
                output_ += ",\n";
                skip_space();
                continue;
            }
            if (!consume('}')) {
                return set_error("PARSE_JSON_OBJECT_END", "Expected ',' or '}' in object");
            }
            output_.push_back('\n');
            append_indent(depth);
            output_.push_back('}');
            return true;
        }
    }

    bool parse_array(int depth) {
        output_.push_back('[');
        ++position_;
        skip_space();
        if (consume(']')) {
            output_.push_back(']');
            return true;
        }

        output_.push_back('\n');
        while (true) {
            append_indent(depth + 1);
            if (!parse_value(depth + 1)) {
                return false;
            }
            skip_space();
            if (consume(',')) {
                output_ += ",\n";
                skip_space();
                continue;
            }
            if (!consume(']')) {
                return set_error("PARSE_JSON_ARRAY_END", "Expected ',' or ']' in array");
            }
            output_.push_back('\n');
            append_indent(depth);
            output_.push_back(']');
            return true;
        }
    }

    bool parse_string() {
        const std::size_t start = position_++;
        while (position_ < input_.size()) {
            const unsigned char current = static_cast<unsigned char>(input_[position_++]);
            if (current == '"') {
                output_.append(input_, start, position_ - start);
                return true;
            }
            if (current < 0x20) {
                return set_error("PARSE_JSON_CONTROL", "Unescaped control character in string");
            }
            if (current != '\\') {
                continue;
            }
            if (position_ >= input_.size()) {
                return set_error("PARSE_JSON_ESCAPE", "Unfinished string escape");
            }
            const char escape = input_[position_++];
            if (escape == 'u') {
                for (int i = 0; i < 4; ++i) {
                    if (position_ >= input_.size() || !is_hex(input_[position_])) {
                        return set_error("PARSE_JSON_UNICODE", "Expected four hexadecimal digits after '\\u'");
                    }
                    ++position_;
                }
            } else if (std::string_view("\"\\/bfnrt").find(escape) == std::string_view::npos) {
                return set_error("PARSE_JSON_ESCAPE", "Invalid JSON string escape");
            }
        }
        return set_error("PARSE_JSON_STRING_END", "Unterminated string");
    }

    bool parse_number() {
        const std::size_t start = position_;
        consume('-');
        if (consume('0')) {
            if (position_ < input_.size() && is_digit(input_[position_])) {
                return set_error("PARSE_JSON_NUMBER", "Leading zeros are not allowed in JSON numbers");
            }
        } else {
            if (position_ >= input_.size() || !is_digit_one_to_nine(input_[position_])) {
                return set_error("PARSE_JSON_NUMBER", "Invalid JSON number");
            }
            while (position_ < input_.size() && is_digit(input_[position_])) {
                ++position_;
            }
        }

        if (consume('.')) {
            if (position_ >= input_.size() || !is_digit(input_[position_])) {
                return set_error("PARSE_JSON_NUMBER", "Expected digits after decimal point");
            }
            while (position_ < input_.size() && is_digit(input_[position_])) {
                ++position_;
            }
        }
        if (position_ < input_.size() && (input_[position_] == 'e' || input_[position_] == 'E')) {
            ++position_;
            if (position_ < input_.size() && (input_[position_] == '+' || input_[position_] == '-')) {
                ++position_;
            }
            if (position_ >= input_.size() || !is_digit(input_[position_])) {
                return set_error("PARSE_JSON_NUMBER", "Expected exponent digits");
            }
            while (position_ < input_.size() && is_digit(input_[position_])) {
                ++position_;
            }
        }
        output_.append(input_, start, position_ - start);
        return true;
    }

    bool parse_literal(std::string_view literal) {
        if (input_.compare(position_, literal.size(), literal) != 0) {
            return set_error("PARSE_JSON_LITERAL", "Invalid JSON literal");
        }
        output_.append(literal.data(), literal.size());
        position_ += literal.size();
        return true;
    }

    void skip_space() {
        while (position_ < input_.size()) {
            const char value = input_[position_];
            if (value != ' ' && value != '\t' && value != '\r' && value != '\n') {
                break;
            }
            ++position_;
        }
    }

    bool consume(char expected) {
        if (position_ < input_.size() && input_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    void append_indent(int depth) {
        output_.append(static_cast<std::size_t>(depth * indent_width_), ' ');
    }

    bool set_error(std::string code, std::string message) {
        if (issue_.code.empty()) {
            issue_.code = std::move(code);
            issue_.message = std::move(message);
            issue_.offset = std::min(position_, input_.size());
            issue_.line = 1;
            issue_.column = 1;
            for (std::size_t i = 0; i < issue_.offset; ++i) {
                if (input_[i] == '\n') {
                    ++issue_.line;
                    issue_.column = 1;
                } else {
                    ++issue_.column;
                }
            }
        }
        return false;
    }

    FormatResult failure() {
        return {false, {}, issue_};
    }

    static bool is_digit(char value) { return value >= '0' && value <= '9'; }
    static bool is_digit_one_to_nine(char value) { return value >= '1' && value <= '9'; }
    static bool is_hex(char value) {
        return is_digit(value) || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
    }

    const std::string& input_;
    int indent_width_;
    std::size_t position_ = 0;
    std::string output_;
    ParseIssue issue_;
};

} // namespace

FormatResult format_json(const std::string& input, int indent_width) {
    return JsonFormatter(input, indent_width).run();
}

} // namespace zeus
