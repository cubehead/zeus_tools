#include "zeus/developer_tools.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>

namespace zeus {
namespace {

ParseIssue issue(const char* code, const char* message, std::size_t offset = 0) {
    ParseIssue value;
    value.code = code;
    value.message = message;
    value.offset = offset;
    value.line = 1;
    value.column = offset + 1;
    return value;
}

void append_utf8(std::string& output, std::uint32_t codepoint) {
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
}

int hex_digit(unsigned char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

std::string trim_ascii(const std::string& input) {
    const auto first = std::find_if_not(input.begin(), input.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    return first < last ? std::string(first, last) : std::string{};
}

bool parse_timestamp(std::string_view input, std::int64_t& milliseconds) {
    while (!input.empty() &&
           std::isspace(static_cast<unsigned char>(input.front())) != 0) {
        input.remove_prefix(1);
    }
    while (!input.empty() &&
           std::isspace(static_cast<unsigned char>(input.back())) != 0) {
        input.remove_suffix(1);
    }
    const std::string_view trimmed = input;
    if (trimmed.size() != 10 && trimmed.size() != 13) return false;
    if (!std::all_of(trimmed.begin(), trimmed.end(), [](unsigned char ch) {
            return std::isdigit(ch) != 0;
        })) return false;
    std::int64_t raw = 0;
    const auto converted = std::from_chars(
        trimmed.data(), trimmed.data() + trimmed.size(), raw);
    if (converted.ec != std::errc{} ||
        converted.ptr != trimmed.data() + trimmed.size()) return false;
    milliseconds = trimmed.size() == 10 ? raw * 1000 : raw;
    return true;
}

bool safe_gmtime(std::time_t value, std::tm& output) {
#if defined(_WIN32)
    return gmtime_s(&output, &value) == 0;
#else
    return gmtime_r(&value, &output) != nullptr;
#endif
}

bool safe_localtime(std::time_t value, std::tm& output) {
#if defined(_WIN32)
    return localtime_s(&output, &value) == 0;
#else
    return localtime_r(&value, &output) != nullptr;
#endif
}

} // namespace

std::string encode_html_entities(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    for (const char ch : input) {
        switch (ch) {
        case '&': output += "&amp;"; break;
        case '<': output += "&lt;"; break;
        case '>': output += "&gt;"; break;
        case '"': output += "&quot;"; break;
        case '\'': output += "&#39;"; break;
        default: output.push_back(ch); break;
        }
    }
    return output;
}

FormatResult decode_html_entities(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    bool decoded_any = false;
    for (std::size_t index = 0; index < input.size();) {
        if (input[index] != '&') {
            output.push_back(input[index++]);
            continue;
        }
        const std::size_t end = input.find(';', index + 1);
        if (end == std::string::npos || end - index > 12) {
            return {false, {}, issue("INVALID_HTML_ENTITY", "Invalid or unfinished HTML entity", index)};
        }
        const std::string entity = input.substr(index + 1, end - index - 1);
        if (entity == "amp") output.push_back('&');
        else if (entity == "lt") output.push_back('<');
        else if (entity == "gt") output.push_back('>');
        else if (entity == "quot") output.push_back('"');
        else if (entity == "apos" || entity == "#39") output.push_back('\'');
        else if (!entity.empty() && entity.front() == '#') {
            const bool hexadecimal = entity.size() > 2 &&
                (entity[1] == 'x' || entity[1] == 'X');
            const std::size_t digits_start = hexadecimal ? 2 : 1;
            if (digits_start >= entity.size()) {
                return {false, {}, issue("INVALID_HTML_ENTITY", "HTML numeric entity has no digits", index)};
            }
            std::uint32_t codepoint = 0;
            for (std::size_t digit_index = digits_start; digit_index < entity.size(); ++digit_index) {
                const int digit = hexadecimal
                    ? hex_digit(static_cast<unsigned char>(entity[digit_index]))
                    : (std::isdigit(static_cast<unsigned char>(entity[digit_index]))
                        ? entity[digit_index] - '0' : -1);
                if (digit < 0) {
                    return {false, {}, issue("INVALID_HTML_ENTITY", "HTML numeric entity contains invalid digits", index)};
                }
                const std::uint32_t base = hexadecimal ? 16U : 10U;
                if (codepoint > (0x10FFFFU - static_cast<std::uint32_t>(digit)) / base) {
                    return {false, {}, issue("INVALID_HTML_ENTITY", "HTML entity code point is out of range", index)};
                }
                codepoint = codepoint * base + static_cast<std::uint32_t>(digit);
            }
            if (codepoint == 0 || codepoint > 0x10FFFFU ||
                (codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
                return {false, {}, issue("INVALID_HTML_ENTITY", "HTML entity code point is not valid Unicode", index)};
            }
            append_utf8(output, codepoint);
        } else {
            return {false, {}, issue("UNSUPPORTED_HTML_ENTITY", "Unsupported named HTML entity", index)};
        }
        decoded_any = true;
        index = end + 1;
    }
    if (!decoded_any) {
        return {false, {}, issue("NO_HTML_ENTITY", "Input does not contain a supported HTML entity")};
    }
    return {true, std::move(output), {}};
}

std::string encode_hex(const std::string& input) {
    constexpr char digits[] = "0123456789abcdef";
    std::string output;
    output.reserve(input.size() * 2);
    for (const unsigned char ch : input) {
        output.push_back(digits[ch >> 4U]);
        output.push_back(digits[ch & 0x0FU]);
    }
    return output;
}

FormatResult decode_hex(const std::string& input) {
    std::string digits;
    digits.reserve(input.size());
    std::size_t index = 0;
    while (index < input.size() && std::isspace(static_cast<unsigned char>(input[index]))) ++index;
    if (index + 1 < input.size() && input[index] == '0' &&
        (input[index + 1] == 'x' || input[index + 1] == 'X')) index += 2;
    for (; index < input.size(); ++index) {
        const unsigned char ch = static_cast<unsigned char>(input[index]);
        if (std::isspace(ch) || ch == ':' || ch == '-') continue;
        if (hex_digit(ch) < 0) {
            return {false, {}, issue("INVALID_HEX", "Hex input contains a non-hexadecimal character", index)};
        }
        digits.push_back(static_cast<char>(ch));
    }
    if (digits.empty() || digits.size() % 2 != 0) {
        return {false, {}, issue("INVALID_HEX_LENGTH", "Hex input must contain a whole number of bytes")};
    }
    std::string output;
    output.reserve(digits.size() / 2);
    for (std::size_t offset = 0; offset < digits.size(); offset += 2) {
        output.push_back(static_cast<char>(
            (hex_digit(static_cast<unsigned char>(digits[offset])) << 4) |
            hex_digit(static_cast<unsigned char>(digits[offset + 1]))));
    }
    return {true, std::move(output), {}};
}

bool looks_like_hex_encoding(const std::string& input) {
    const std::string trimmed = trim_ascii(input);
    if (trimmed.size() < 4) return false;
    const bool explicit_prefix = trimmed.size() > 2 && trimmed[0] == '0' &&
        (trimmed[1] == 'x' || trimmed[1] == 'X');
    const bool explicit_separator = trimmed.find_first_of(" :-") != std::string::npos;
    if (!explicit_prefix && !explicit_separator) return false;
    return decode_hex(trimmed).ok;
}

bool looks_like_unix_timestamp(std::string_view input) {
    std::int64_t ignored = 0;
    return parse_timestamp(input, ignored);
}

FormatResult format_unix_timestamp(const std::string& input) {
    std::int64_t milliseconds = 0;
    if (!parse_timestamp(input, milliseconds)) {
        return {false, {}, issue("INVALID_TIMESTAMP", "Unix timestamp must contain exactly 10 seconds digits or 13 milliseconds digits")};
    }
    const std::int64_t seconds = milliseconds / 1000;
    const int millisecond_part = static_cast<int>(milliseconds % 1000);
    if (seconds < static_cast<std::int64_t>(std::numeric_limits<std::time_t>::min()) ||
        seconds > static_cast<std::int64_t>(std::numeric_limits<std::time_t>::max())) {
        return {false, {}, issue("TIMESTAMP_RANGE", "Unix timestamp is outside the platform time range")};
    }
    const std::time_t time = static_cast<std::time_t>(seconds);
    std::tm utc{};
    std::tm local{};
    if (!safe_gmtime(time, utc) || !safe_localtime(time, local)) {
        return {false, {}, issue("TIMESTAMP_RANGE", "Unix timestamp could not be converted on this platform")};
    }
    std::ostringstream output;
    output << "Unix seconds: " << seconds << '\n'
           << "Unix milliseconds: " << milliseconds << '\n'
           << "UTC: " << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S") << '.'
           << std::setw(3) << std::setfill('0') << millisecond_part << "Z\n"
           << "Local: " << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << '.'
           << std::setw(3) << std::setfill('0') << millisecond_part;
    return {true, output.str(), {}};
}

} // namespace zeus
