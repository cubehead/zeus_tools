#include "zeus/text_processor.h"

#include "zeus/json_formatter.h"
#include "zeus/structured_converter.h"
#include "zeus/csv_document.h"
#include "zeus/xml_formatter.h"
#include "zeus/yaml_formatter.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <vector>

namespace zeus {
namespace {

std::string trim_ascii(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    return first < last ? std::string(first, last) : std::string{};
}

bool is_valid_utf8(const std::string& value) {
    for (std::size_t i = 0; i < value.size();) {
        const auto lead = static_cast<unsigned char>(value[i]);
        std::size_t continuation = 0;
        std::uint32_t codepoint = 0;
        if (lead <= 0x7FU) {
            codepoint = lead;
        } else if ((lead & 0xE0U) == 0xC0U) {
            continuation = 1;
            codepoint = lead & 0x1FU;
            if (codepoint < 2) return false;
        } else if ((lead & 0xF0U) == 0xE0U) {
            continuation = 2;
            codepoint = lead & 0x0FU;
        } else if ((lead & 0xF8U) == 0xF0U && lead <= 0xF4U) {
            continuation = 3;
            codepoint = lead & 0x07U;
        } else {
            return false;
        }
        if (i + continuation >= value.size()) return false;
        for (std::size_t j = 1; j <= continuation; ++j) {
            const auto next = static_cast<unsigned char>(value[i + j]);
            if ((next & 0xC0U) != 0x80U) return false;
            codepoint = (codepoint << 6U) | (next & 0x3FU);
        }
        if ((continuation == 2 && codepoint < 0x800U) ||
            (continuation == 3 && codepoint < 0x10000U) ||
            (codepoint >= 0xD800U && codepoint <= 0xDFFFU) || codepoint > 0x10FFFFU) {
            return false;
        }
        i += continuation + 1;
    }
    return true;
}

bool is_displayable_text(const std::string& value) {
    if (value.empty() || !is_valid_utf8(value)) return false;
    for (unsigned char ch : value) {
        if (ch == 0 || (ch < 0x20U && ch != '\n' && ch != '\r' && ch != '\t')) return false;
    }
    return true;
}

int base64_value(unsigned char ch) {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
    if (ch >= '0' && ch <= '9') return ch - '0' + 52;
    if (ch == '+' || ch == '-') return 62;
    if (ch == '/' || ch == '_') return 63;
    return -1;
}

bool decode_base64(const std::string& input, std::string& output) {
    std::string compact;
    compact.reserve(input.size());
    for (unsigned char ch : input) {
        if (std::isspace(ch)) continue;
        compact.push_back(static_cast<char>(ch));
    }
    if (compact.empty() || compact.size() % 4 == 1) return false;
    const auto padding_at = compact.find('=');
    if (padding_at != std::string::npos) {
        if (compact.size() % 4 != 0 || compact.size() - padding_at > 2) return false;
        if (!std::all_of(compact.begin() + static_cast<std::ptrdiff_t>(padding_at), compact.end(),
                         [](char ch) { return ch == '='; })) return false;
    }

    output.clear();
    std::uint32_t buffer = 0;
    int bits = 0;
    for (unsigned char ch : compact) {
        if (ch == '=') break;
        const int value = base64_value(ch);
        if (value < 0) return false;
        buffer = (buffer << 6U) | static_cast<std::uint32_t>(value);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            output.push_back(static_cast<char>((buffer >> bits) & 0xFFU));
        }
    }
    if (bits != 0 && (buffer & ((1U << bits) - 1U)) != 0) return false;
    return true;
}

std::string encode_base64(const std::string& input) {
    constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);
    for (std::size_t i = 0; i < input.size(); i += 3) {
        const auto a = static_cast<unsigned char>(input[i]);
        const auto b = i + 1 < input.size() ? static_cast<unsigned char>(input[i + 1]) : 0U;
        const auto c = i + 2 < input.size() ? static_cast<unsigned char>(input[i + 2]) : 0U;
        const std::uint32_t value = (static_cast<std::uint32_t>(a) << 16U) |
                                    (static_cast<std::uint32_t>(b) << 8U) |
                                    static_cast<std::uint32_t>(c);
        output.push_back(alphabet[(value >> 18U) & 0x3FU]);
        output.push_back(alphabet[(value >> 12U) & 0x3FU]);
        output.push_back(i + 1 < input.size() ? alphabet[(value >> 6U) & 0x3FU] : '=');
        output.push_back(i + 2 < input.size() ? alphabet[value & 0x3FU] : '=');
    }
    return output;
}

int hex_value(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

bool decode_url(const std::string& input, std::string& output, std::size_t& encoded_count) {
    output.clear();
    output.reserve(input.size());
    encoded_count = 0;
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '%') {
            if (i + 2 >= input.size()) return false;
            const int high = hex_value(input[i + 1]);
            const int low = hex_value(input[i + 2]);
            if (high < 0 || low < 0) return false;
            output.push_back(static_cast<char>((high << 4) | low));
            i += 2;
            ++encoded_count;
        } else if (input[i] == '+') {
            output.push_back(' ');
        } else {
            output.push_back(input[i]);
        }
    }
    return true;
}

std::string encode_url(const std::string& input) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string output;
    output.reserve(input.size() * 3);
    for (unsigned char ch : input) {
        if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            output.push_back(static_cast<char>(ch));
        } else {
            output.push_back('%');
            output.push_back(hex[(ch >> 4U) & 0x0FU]);
            output.push_back(hex[ch & 0x0FU]);
        }
    }
    return output;
}

std::string minify_json(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    bool in_string = false;
    bool escaped = false;
    for (unsigned char ch : input) {
        if (in_string) {
            output.push_back(static_cast<char>(ch));
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                in_string = false;
            }
        } else if (ch == '"') {
            in_string = true;
            output.push_back('"');
        } else if (!std::isspace(ch)) {
            output.push_back(static_cast<char>(ch));
        }
    }
    return output;
}

std::string escape_json_text(const std::string& input) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string output;
    output.reserve(input.size() + input.size() / 4);
    for (unsigned char ch : input) {
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
        }
    }
    return output;
}

std::string binary_summary(const std::string& value) {
    std::ostringstream stream;
    stream << "Binary data · " << value.size() << " bytes\nHex preview: ";
    const std::size_t preview_size = std::min<std::size_t>(value.size(), 64);
    for (std::size_t i = 0; i < preview_size; ++i) {
        if (i != 0) stream << ' ';
        stream << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(static_cast<unsigned char>(value[i]));
    }
    if (value.size() > preview_size) stream << " …";
    return stream.str();
}

bool inspect_escaped_json(const std::string& input, std::string& output) {
    const std::size_t first = input.find_first_not_of(" \t\r\n");
    const std::size_t last = input.find_last_not_of(" \t\r\n");
    const bool quoted_json_string = first != std::string::npos &&
        last != std::string::npos && input[first] == '"' && input[last] == '"';
    if (!quoted_json_string && input.find("\\\"") == std::string::npos) {
        return false;
    }
    const auto unescaped = unescape_json_string(input);
    if (!unescaped.ok) return false;
    const auto nested = format_json(unescaped.value, 2);
    if (!nested.ok) return false;
    output = nested.value;
    return true;
}

std::string indent_json_member(const std::string& value) {
    std::string output;
    output.reserve(value.size() + 16);
    for (char ch : value) {
        output.push_back(ch);
        if (ch == '\n') output += "  ";
    }
    return output;
}

bool is_base64url_segment(const std::string& value, bool allow_empty = false) {
    if (value.empty()) return allow_empty;
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isalnum(ch) || ch == '-' || ch == '_';
    });
}

bool inspect_jwt(const std::string& input, std::string& output) {
    if (input.find_first_of(" \t\r\n") != std::string::npos) return false;
    const std::size_t first_dot = input.find('.');
    if (first_dot == std::string::npos) return false;
    const std::size_t second_dot = input.find('.', first_dot + 1);
    if (second_dot == std::string::npos || input.find('.', second_dot + 1) != std::string::npos) return false;

    const std::string header_segment = input.substr(0, first_dot);
    const std::string payload_segment = input.substr(first_dot + 1, second_dot - first_dot - 1);
    const std::string signature_segment = input.substr(second_dot + 1);
    if (!is_base64url_segment(header_segment) || !is_base64url_segment(payload_segment) ||
        !is_base64url_segment(signature_segment, true)) return false;

    std::string header;
    std::string payload;
    if (!decode_base64(header_segment, header) || !decode_base64(payload_segment, payload) ||
        !is_displayable_text(header) || !is_displayable_text(payload)) return false;

    const auto formatted_header = format_json(header, 2);
    const auto formatted_payload = format_json(payload, 2);
    if (!formatted_header.ok || !formatted_payload.ok ||
        formatted_header.value.empty() || formatted_header.value.front() != '{' ||
        formatted_payload.value.empty() || formatted_payload.value.front() != '{') return false;

    output = "{\n  \"header\": ";
    output += indent_json_member(formatted_header.value);
    output += ",\n  \"payload\": ";
    output += indent_json_member(formatted_payload.value);
    output += ",\n  \"signature\": \"";
    output += signature_segment;
    output += "\",\n  \"verification\": \"not verified\"\n}";
    return true;
}

ProcessResult decoded_result(ContentKind source_kind, const char* source_label, std::string decoded) {
    ProcessResult result;
    result.detected = source_kind;
    result.decoded = true;
    const auto json = format_json(decoded, 2);
    if (json.ok) {
        result.label = std::string(source_label) + " → JSON";
        result.value = json.value;
        result.structured = true;
    } else if (is_displayable_text(decoded)) {
        result.label = std::string(source_label) + " → Text";
        result.value = std::move(decoded);
    } else {
        result.label = std::string(source_label) + " → Binary";
        result.value = binary_summary(decoded);
    }
    return result;
}

ProcessResult failure(ContentKind kind, const char* code, const char* message) {
    ProcessResult result;
    result.ok = false;
    result.detected = kind;
    result.label = content_kind_name(kind);
    result.error_code = code;
    result.error_message = message;
    return result;
}

} // namespace

const char* content_kind_name(ContentKind kind) {
    switch (kind) {
    case ContentKind::Empty: return "Empty";
    case ContentKind::Json: return "JSON";
    case ContentKind::Xml: return "XML";
    case ContentKind::Yaml: return "YAML";
    case ContentKind::Jwt: return "JWT";
    case ContentKind::JsonEscaped: return "JSON String";
    case ContentKind::Base64: return "Base64";
    case ContentKind::UrlEncoded: return "URL Encode";
    case ContentKind::Csv: return "CSV";
    case ContentKind::Text: return "Text";
    }
    return "Text";
}

ProcessResult process_text(const std::string& input, ProcessingMode mode) {
    const std::string trimmed = trim_ascii(input);
    if (trimmed.empty()) {
        ProcessResult result;
        result.detected = ContentKind::Empty;
        result.label = "Empty";
        return result;
    }

    if (mode == ProcessingMode::Base64Encode) {
        ProcessResult result;
        result.detected = ContentKind::Text;
        result.label = "Base64 Encode";
        result.value = encode_base64(input);
        return result;
    }

    if (mode == ProcessingMode::JsonMinify) {
        const auto json = format_json(input, 2);
        if (!json.ok) {
            ProcessResult result = failure(ContentKind::Json, json.issue.code.c_str(), json.issue.message.c_str());
            result.value = input;
            result.error_line = json.issue.line;
            result.error_column = json.issue.column;
            return result;
        }
        ProcessResult result;
        result.detected = ContentKind::Json;
        result.label = "JSON Minify";
        result.value = minify_json(input);
        result.structured = true;
        return result;
    }

    if (mode == ProcessingMode::JsonEscape) {
        ProcessResult result;
        result.detected = ContentKind::Text;
        result.label = "JSON Escape";
        result.value = escape_json_text(input);
        return result;
    }

    if (mode == ProcessingMode::JsonUnescape) {
        std::string unescaped;
        if (!inspect_escaped_json(input, unescaped)) {
            ProcessResult result = failure(ContentKind::JsonEscaped, "INVALID_ESCAPED_JSON",
                "Input does not contain one layer of escaped JSON");
            result.value = input;
            return result;
        }
        ProcessResult result;
        result.detected = ContentKind::Json;
        result.label = "JSON Unescape";
        result.value = std::move(unescaped);
        result.decoded = true;
        result.structured = true;
        return result;
    }

    if (mode == ProcessingMode::JsonToYaml || mode == ProcessingMode::JsonToXml ||
        mode == ProcessingMode::XmlToJson ||
        mode == ProcessingMode::JsonToCsv ||
        mode == ProcessingMode::YamlToJson) {
        const auto converted = mode == ProcessingMode::JsonToYaml
            ? json_to_yaml(input, 2)
            : mode == ProcessingMode::JsonToXml
                ? json_to_xml(input, 2)
                : mode == ProcessingMode::XmlToJson
                    ? xml_to_json(input, 2)
                : mode == ProcessingMode::JsonToCsv
                    ? json_to_csv(input)
                : yaml_to_json(input, 2);
        const ContentKind source = mode == ProcessingMode::YamlToJson
            ? ContentKind::Yaml
            : mode == ProcessingMode::XmlToJson ? ContentKind::Xml : ContentKind::Json;
        if (!converted.ok) {
            ProcessResult result = failure(source, converted.issue.code.c_str(), converted.issue.message.c_str());
            result.value = input;
            result.error_line = converted.issue.line;
            result.error_column = converted.issue.column;
            return result;
        }
        ProcessResult result;
        result.detected = mode == ProcessingMode::JsonToYaml ? ContentKind::Yaml
            : mode == ProcessingMode::JsonToXml ? ContentKind::Xml
            : mode == ProcessingMode::XmlToJson ? ContentKind::Json
            : mode == ProcessingMode::JsonToCsv ? ContentKind::Csv : ContentKind::Json;
        result.label = mode == ProcessingMode::JsonToYaml ? "JSON → YAML"
            : mode == ProcessingMode::JsonToXml ? "JSON → XML"
            : mode == ProcessingMode::XmlToJson ? "XML → JSON"
            : mode == ProcessingMode::JsonToCsv ? "JSON → CSV" : "YAML → JSON";
        result.value = converted.value;
        result.structured = mode != ProcessingMode::JsonToCsv;
        result.tabular = mode == ProcessingMode::JsonToCsv;
        return result;
    }

    if (mode == ProcessingMode::Upper || mode == ProcessingMode::Lower) {
        ProcessResult result;
        result.detected = ContentKind::Text;
        result.label = mode == ProcessingMode::Upper ? "Upper" : "Lower";
        result.value = input;
        std::transform(result.value.begin(), result.value.end(), result.value.begin(), [mode](unsigned char ch) {
            return static_cast<char>(mode == ProcessingMode::Upper ? std::toupper(ch) : std::tolower(ch));
        });
        return result;
    }

    if (mode == ProcessingMode::UrlEncode) {
        ProcessResult result;
        result.detected = ContentKind::Text;
        result.label = "URL Encode";
        result.value = encode_url(input);
        return result;
    }

    if (mode == ProcessingMode::DecodeOneLayer) {
        std::string unescaped;
        if (inspect_escaped_json(input, unescaped)) {
            ProcessResult result;
            result.detected = ContentKind::Json;
            result.label = "JSON Unescape";
            result.value = std::move(unescaped);
            result.decoded = true;
            result.structured = true;
            return result;
        }

        std::string decoded;
        std::size_t encoded_count = 0;
        if (decode_url(input, decoded, encoded_count) && encoded_count > 0 &&
            is_displayable_text(decoded)) {
            return decoded_result(ContentKind::UrlEncoded, "URL Decode", std::move(decoded));
        }

        decoded.clear();
        if (decode_base64(trimmed, decoded)) {
            return decoded_result(ContentKind::Base64, "Base64", std::move(decoded));
        }

        ProcessResult result = failure(
            ContentKind::Text,
            "NO_ENCODED_LAYER",
            "Current result is not a supported encoded layer");
        result.value = input;
        return result;
    }

    if (mode == ProcessingMode::Auto || mode == ProcessingMode::Json) {
        const auto json = format_json(input, 2);
        if (json.ok) {
            if (mode == ProcessingMode::Auto) {
                std::string unescaped;
                if (inspect_escaped_json(input, unescaped)) {
                    ProcessResult result;
                    result.detected = ContentKind::JsonEscaped;
                    result.label = "JSON String → JSON";
                    result.value = std::move(unescaped);
                    result.decoded = true;
                    result.structured = true;
                    return result;
                }
            }
            ProcessResult result;
            result.detected = ContentKind::Json;
            result.label = "JSON";
            result.value = json.value;
            result.structured = true;
            return result;
        }
        if (mode == ProcessingMode::Json) {
            ProcessResult result = failure(ContentKind::Json, json.issue.code.c_str(), json.issue.message.c_str());
            result.value = input;
            result.error_line = json.issue.line;
            result.error_column = json.issue.column;
            return result;
        }
    }

    if (mode == ProcessingMode::Auto) {
        std::string unescaped;
        if (inspect_escaped_json(trimmed, unescaped)) {
            ProcessResult result;
            result.detected = ContentKind::JsonEscaped;
            result.label = "Escaped JSON → JSON";
            result.value = std::move(unescaped);
            result.decoded = true;
            result.structured = true;
            return result;
        }
    }

    if (mode == ProcessingMode::Auto || mode == ProcessingMode::Xml) {
        const bool looks_like_xml = !trimmed.empty() && trimmed.front() == '<';
        if (looks_like_xml || mode == ProcessingMode::Xml) {
            const auto xml = format_xml(input, 2);
            if (xml.ok) {
                ProcessResult result;
                result.detected = ContentKind::Xml;
                result.label = "XML";
                result.value = xml.value;
                result.structured = true;
                return result;
            }
            if (mode == ProcessingMode::Xml || looks_like_xml) {
                ProcessResult result = failure(ContentKind::Xml, xml.issue.code.c_str(), xml.issue.message.c_str());
                result.value = input;
                result.error_line = xml.issue.line;
                result.error_column = xml.issue.column;
                return result;
            }
        }
    }

    if (mode == ProcessingMode::Auto) {
        std::string inspected;
        if (inspect_jwt(trimmed, inspected)) {
            ProcessResult result;
            result.detected = ContentKind::Jwt;
            result.label = "JWT · Unverified";
            result.value = std::move(inspected);
            result.decoded = true;
            result.structured = true;
            return result;
        }
    }

    if (mode == ProcessingMode::Auto || mode == ProcessingMode::Yaml) {
        const bool candidate = looks_like_yaml(input);
        if (candidate || mode == ProcessingMode::Yaml) {
            const auto yaml = format_yaml(input, 2);
            if (yaml.ok) {
                ProcessResult result;
                result.detected = ContentKind::Yaml;
                result.label = "YAML";
                result.value = yaml.value;
                result.structured = true;
                return result;
            }
            if (mode == ProcessingMode::Yaml) {
                ProcessResult result = failure(ContentKind::Yaml, yaml.issue.code.c_str(), yaml.issue.message.c_str());
                result.value = input;
                result.error_line = yaml.issue.line;
                result.error_column = yaml.issue.column;
                return result;
            }
        }
    }

    if (mode == ProcessingMode::Auto || mode == ProcessingMode::Csv) {
        const auto csv = parse_csv(input, '\0', true);
        if (csv.ok) {
            ProcessResult result;
            result.detected = ContentKind::Csv;
            result.label = "CSV";
            result.value = csv.document.to_tsv();
            result.tabular = true;
            return result;
        }
        if (mode == ProcessingMode::Csv) {
            ProcessResult result = failure(ContentKind::Csv, "INVALID_CSV", csv.error.c_str());
            result.value = input;
            return result;
        }
    }

    if (mode == ProcessingMode::Auto || mode == ProcessingMode::UrlDecode) {
        std::string decoded;
        std::size_t encoded_count = 0;
        const bool valid = decode_url(input, decoded, encoded_count);
        const bool has_manual_encoding = encoded_count > 0 || input.find('+') != std::string::npos;
        if (valid && ((mode == ProcessingMode::UrlDecode && has_manual_encoding) ||
                      (encoded_count > 0 && is_displayable_text(decoded)))) {
            return decoded_result(ContentKind::UrlEncoded, "URL Decode", std::move(decoded));
        }
        if (mode == ProcessingMode::UrlDecode) {
            return failure(ContentKind::UrlEncoded, "INVALID_URL_ENCODING",
                           "URL encoded input contains an invalid percent sequence");
        }
    }

    if (mode == ProcessingMode::Auto || mode == ProcessingMode::Base64) {
        std::string decoded;
        const bool valid = decode_base64(trimmed, decoded);
        const bool confident = trimmed.size() >= 12 && is_displayable_text(decoded);
        if (valid && (mode == ProcessingMode::Base64 || confident)) {
            return decoded_result(ContentKind::Base64, "Base64", std::move(decoded));
        }
        if (mode == ProcessingMode::Base64) {
            return failure(ContentKind::Base64, "INVALID_BASE64", "Input is not valid standard or URL-safe Base64");
        }
    }

    ProcessResult result;
    result.detected = ContentKind::Text;
    result.label = "Text";
    result.value = input;
    return result;
}

} // namespace zeus
