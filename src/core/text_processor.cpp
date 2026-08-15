#include "zeus/text_processor.h"

#include "zeus/json_formatter.h"
#include "zeus/structured_converter.h"
#include "zeus/csv_document.h"
#include "zeus/developer_tools.h"
#include "zeus/ini_formatter.h"
#include "zeus/toml_formatter.h"
#include "zeus/xml_formatter.h"
#include "zeus/yaml_formatter.h"

#include <algorithm>
#include <array>
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
        result.output_kind = ContentKind::Json;
        result.label = std::string(source_label) + " → JSON";
        result.value = json.value;
        result.structured = true;
    } else if (is_displayable_text(decoded)) {
        result.output_kind = ContentKind::Text;
        result.label = std::string(source_label) + " → Text";
        result.value = std::move(decoded);
    } else {
        result.output_kind = ContentKind::Text;
        result.label = std::string(source_label) + " → Binary";
        result.value = binary_summary(decoded);
    }
    return result;
}

ProcessResult failure(ContentKind kind, const char* code, const char* message) {
    ProcessResult result;
    result.ok = false;
    result.detected = kind;
    result.output_kind = kind;
    result.label = content_kind_name(kind);
    result.error_code = code;
    result.error_message = message;
    return result;
}

ProcessResult format_failure(ContentKind kind, const FormatResult& formatted,
                             const std::string& input) {
    ProcessResult result = failure(
        kind, formatted.issue.code.c_str(), formatted.issue.message.c_str());
    result.value = input;
    result.error_line = formatted.issue.line;
    result.error_column = formatted.issue.column;
    return result;
}

ProcessResult run_text_transform(
    const std::string& input,
    const std::string&,
    ProcessingMode mode) {
    ProcessResult result;
    result.detected = ContentKind::Text;
    result.output_kind = ContentKind::Text;
    if (mode == ProcessingMode::Base64Encode) {
        result.output_kind = ContentKind::Base64;
        result.label = "Base64 Encode";
        result.value = encode_base64(input);
    } else if (mode == ProcessingMode::HtmlEntityEncode) {
        result.output_kind = ContentKind::HtmlEntity;
        result.label = "HTML Entity Encode";
        result.value = encode_html_entities(input);
    } else if (mode == ProcessingMode::HexEncode) {
        result.output_kind = ContentKind::HexEncoded;
        result.label = "Hex Encode";
        result.value = encode_hex(input);
    } else if (mode == ProcessingMode::JsonEscape) {
        result.output_kind = ContentKind::JsonEscaped;
        result.label = "JSON Escape";
        result.value = escape_json_text(input);
    } else if (mode == ProcessingMode::UrlEncode) {
        result.output_kind = ContentKind::UrlEncoded;
        result.label = "URL Encode";
        result.value = encode_url(input);
    } else if (mode == ProcessingMode::Upper || mode == ProcessingMode::Lower) {
        result.label = mode == ProcessingMode::Upper ? "Upper" : "Lower";
        result.value = input;
        std::transform(result.value.begin(), result.value.end(), result.value.begin(),
            [mode](unsigned char ch) {
                return static_cast<char>(mode == ProcessingMode::Upper
                    ? std::toupper(ch) : std::tolower(ch));
            });
    } else if (mode == ProcessingMode::Timestamp) {
        const auto converted = format_unix_timestamp(input);
        if (!converted.ok) {
            result = format_failure(ContentKind::Text, converted, input);
        } else {
            result.label = "Unix Time";
            result.value = converted.value;
        }
    } else {
        result.label = "Text";
        result.value = input;
    }
    return result;
}

ProcessResult run_json_processor(
    const std::string& input,
    const std::string&,
    ProcessingMode mode) {
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
        result.output_kind = ContentKind::Json;
        result.label = "JSON Unescape";
        result.value = std::move(unescaped);
        result.decoded = true;
        result.structured = true;
        return result;
    }

    const auto json = format_json(input, 2);
    if (!json.ok) return format_failure(ContentKind::Json, json, input);
    ProcessResult result;
    result.detected = ContentKind::Json;
    result.output_kind = ContentKind::Json;
    result.label = mode == ProcessingMode::JsonMinify ? "JSON Minify" : "JSON";
    result.value = mode == ProcessingMode::JsonMinify ? minify_json(input) : json.value;
    result.structured = true;
    return result;
}

ProcessResult run_conversion_processor(
    const std::string& input,
    const std::string&,
    ProcessingMode mode) {
    FormatResult converted;
    ContentKind source = ContentKind::Json;
    ContentKind target = ContentKind::Json;
    const char* label = "JSON";
    bool tabular = false;
    switch (mode) {
    case ProcessingMode::JsonToYaml:
        converted = json_to_yaml(input, 2);
        target = ContentKind::Yaml;
        label = "JSON → YAML";
        break;
    case ProcessingMode::JsonToXml:
        converted = json_to_xml(input, 2);
        target = ContentKind::Xml;
        label = "JSON → XML";
        break;
    case ProcessingMode::JsonToCsv:
        converted = json_to_csv(input);
        target = ContentKind::Csv;
        label = "JSON → CSV";
        tabular = true;
        break;
    case ProcessingMode::XmlToJson:
        converted = xml_to_json(input, 2);
        source = ContentKind::Xml;
        label = "XML → JSON";
        break;
    case ProcessingMode::YamlToJson:
        converted = yaml_to_json(input, 2);
        source = ContentKind::Yaml;
        label = "YAML → JSON";
        break;
    case ProcessingMode::TomlToJson:
        converted = toml_to_json(input, 2);
        source = ContentKind::Toml;
        label = "TOML → JSON";
        break;
    case ProcessingMode::JsonToToml:
        converted = json_to_toml(input);
        target = ContentKind::Toml;
        label = "JSON → TOML";
        break;
    case ProcessingMode::IniToJson:
        converted = ini_to_json(input, 2);
        source = ContentKind::Ini;
        label = "INI → JSON";
        break;
    default:
        return failure(ContentKind::Text, "UNREGISTERED_CONVERSION",
                       "The requested conversion is not registered");
    }
    if (!converted.ok) return format_failure(source, converted, input);
    ProcessResult result;
    result.detected = target;
    result.output_kind = target;
    result.label = label;
    result.value = converted.value;
    result.structured = !tabular;
    result.tabular = tabular;
    return result;
}

ProcessResult run_format_processor(
    const std::string& input,
    const std::string&,
    ProcessingMode mode) {
    if (mode == ProcessingMode::Csv) {
        const auto csv = parse_csv(input, '\0', true);
        if (!csv.ok) {
            ProcessResult result = failure(ContentKind::Csv, "INVALID_CSV", csv.error.c_str());
            result.value = input;
            return result;
        }
        ProcessResult result;
        result.detected = ContentKind::Csv;
        result.output_kind = ContentKind::Csv;
        result.label = "CSV";
        result.value = csv.document.to_tsv();
        result.tabular = true;
        return result;
    }

    FormatResult formatted;
    ContentKind kind = ContentKind::Text;
    const char* label = "Text";
    switch (mode) {
    case ProcessingMode::Xml:
        formatted = format_xml(input, 2);
        kind = ContentKind::Xml;
        label = "XML";
        break;
    case ProcessingMode::Yaml:
        formatted = format_yaml(input, 2);
        kind = ContentKind::Yaml;
        label = "YAML";
        break;
    case ProcessingMode::Toml:
        formatted = format_toml(input);
        kind = ContentKind::Toml;
        label = "TOML";
        break;
    case ProcessingMode::Ini:
        formatted = format_ini(input);
        kind = ContentKind::Ini;
        label = "INI / Properties";
        break;
    default:
        return failure(ContentKind::Text, "UNREGISTERED_FORMAT",
                       "The requested format processor is not registered");
    }
    if (!formatted.ok) return format_failure(kind, formatted, input);
    ProcessResult result;
    result.detected = kind;
    result.output_kind = kind;
    result.label = label;
    result.value = formatted.value;
    result.structured = true;
    return result;
}

ProcessResult run_codec_processor(
    const std::string& input,
    const std::string& trimmed,
    ProcessingMode mode) {
    if (mode == ProcessingMode::DecodeOneLayer) {
        std::string unescaped;
        if (inspect_escaped_json(input, unescaped)) {
            ProcessResult result;
            result.detected = ContentKind::Json;
            result.output_kind = ContentKind::Json;
            result.label = "JSON Unescape";
            result.value = std::move(unescaped);
            result.decoded = true;
            result.structured = true;
            return result;
        }
        const auto html = decode_html_entities(input);
        if (html.ok) return decoded_result(ContentKind::HtmlEntity, "HTML Entity", html.value);
        if (looks_like_hex_encoding(input)) {
            const auto hex = decode_hex(input);
            if (hex.ok) return decoded_result(ContentKind::HexEncoded, "Hex", hex.value);
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
        ProcessResult result = failure(ContentKind::Text, "NO_ENCODED_LAYER",
            "Current result is not a supported encoded layer");
        result.value = input;
        return result;
    }

    if (mode == ProcessingMode::UrlDecode) {
        std::string decoded;
        std::size_t encoded_count = 0;
        const bool valid = decode_url(input, decoded, encoded_count);
        const bool has_encoding = encoded_count > 0 || input.find('+') != std::string::npos;
        if (valid && has_encoding) {
            return decoded_result(ContentKind::UrlEncoded, "URL Decode", std::move(decoded));
        }
        return failure(ContentKind::UrlEncoded, "INVALID_URL_ENCODING",
                       "URL encoded input contains an invalid percent sequence");
    }

    if (mode == ProcessingMode::Base64) {
        std::string decoded;
        if (decode_base64(trimmed, decoded)) {
            return decoded_result(ContentKind::Base64, "Base64", std::move(decoded));
        }
        return failure(ContentKind::Base64, "INVALID_BASE64",
                       "Input is not valid standard or URL-safe Base64");
    }

    const bool html = mode == ProcessingMode::HtmlEntityDecode;
    const auto decoded = html ? decode_html_entities(input) : decode_hex(input);
    const ContentKind kind = html ? ContentKind::HtmlEntity : ContentKind::HexEncoded;
    if (!decoded.ok) return format_failure(kind, decoded, input);
    return decoded_result(kind, html ? "HTML Entity" : "Hex", decoded.value);
}

using ModeHandler = ProcessResult (*)(
    const std::string&, const std::string&, ProcessingMode);

struct ProcessorRegistration {
    ProcessingMode mode;
    const char* id;
    ModeHandler handler;
};

constexpr std::array<ProcessorRegistration,
    static_cast<std::size_t>(ProcessingMode::Count) - 1> kProcessors{{
    {ProcessingMode::Json, "json.format", run_json_processor},
    {ProcessingMode::JsonMinify, "json.minify", run_json_processor},
    {ProcessingMode::JsonEscape, "json.escape", run_text_transform},
    {ProcessingMode::JsonUnescape, "json.unescape", run_json_processor},
    {ProcessingMode::JsonToYaml, "json.to_yaml", run_conversion_processor},
    {ProcessingMode::JsonToXml, "json.to_xml", run_conversion_processor},
    {ProcessingMode::JsonToCsv, "json.to_csv", run_conversion_processor},
    {ProcessingMode::Xml, "xml.format", run_format_processor},
    {ProcessingMode::XmlToJson, "xml.to_json", run_conversion_processor},
    {ProcessingMode::Yaml, "yaml.format", run_format_processor},
    {ProcessingMode::YamlToJson, "yaml.to_json", run_conversion_processor},
    {ProcessingMode::Toml, "toml.format", run_format_processor},
    {ProcessingMode::TomlToJson, "toml.to_json", run_conversion_processor},
    {ProcessingMode::JsonToToml, "json.to_toml", run_conversion_processor},
    {ProcessingMode::Ini, "ini.format", run_format_processor},
    {ProcessingMode::IniToJson, "ini.to_json", run_conversion_processor},
    {ProcessingMode::Base64, "base64.decode", run_codec_processor},
    {ProcessingMode::Base64Encode, "base64.encode", run_text_transform},
    {ProcessingMode::UrlDecode, "url.decode", run_codec_processor},
    {ProcessingMode::UrlEncode, "url.encode", run_text_transform},
    {ProcessingMode::DecodeOneLayer, "decode.one_layer", run_codec_processor},
    {ProcessingMode::Csv, "csv.table", run_format_processor},
    {ProcessingMode::Text, "text", run_text_transform},
    {ProcessingMode::Upper, "text.upper", run_text_transform},
    {ProcessingMode::Lower, "text.lower", run_text_transform},
    {ProcessingMode::HtmlEntityDecode, "html.decode", run_codec_processor},
    {ProcessingMode::HtmlEntityEncode, "html.encode", run_text_transform},
    {ProcessingMode::HexDecode, "hex.decode", run_codec_processor},
    {ProcessingMode::HexEncode, "hex.encode", run_text_transform},
    {ProcessingMode::Timestamp, "timestamp.inspect", run_text_transform},
}};

constexpr bool processor_modes_are_complete() {
    std::array<bool, static_cast<std::size_t>(ProcessingMode::Count)> seen{};
    for (const auto& processor : kProcessors) {
        const auto index = static_cast<std::size_t>(processor.mode);
        if (processor.mode == ProcessingMode::Auto ||
            processor.mode == ProcessingMode::Count || seen[index]) {
            return false;
        }
        seen[index] = true;
    }
    for (std::size_t index = 1; index < seen.size(); ++index) {
        if (!seen[index]) return false;
    }
    return true;
}

static_assert(processor_modes_are_complete(),
              "Each explicit processing mode must have exactly one registered handler");

ProcessResult detect_automatically(const std::string& input, const std::string& trimmed) {
    const auto json = format_json(input, 2);
    if (json.ok) {
        std::string unescaped;
        if (inspect_escaped_json(input, unescaped)) {
            ProcessResult result;
            result.detected = ContentKind::JsonEscaped;
            result.output_kind = ContentKind::Json;
            result.label = "JSON String → JSON";
            result.value = std::move(unescaped);
            result.decoded = true;
            result.structured = true;
            return result;
        }
        ProcessResult result;
        result.detected = ContentKind::Json;
        result.output_kind = ContentKind::Json;
        result.label = "JSON";
        result.value = json.value;
        result.structured = true;
        return result;
    }

    std::string unescaped;
    if (inspect_escaped_json(trimmed, unescaped)) {
        ProcessResult result;
        result.detected = ContentKind::JsonEscaped;
        result.output_kind = ContentKind::Json;
        result.label = "Escaped JSON → JSON";
        result.value = std::move(unescaped);
        result.decoded = true;
        result.structured = true;
        return result;
    }

    if (!trimmed.empty() && trimmed.front() == '<') {
        const auto xml = format_xml(input, 2);
        if (!xml.ok) return format_failure(ContentKind::Xml, xml, input);
        ProcessResult result;
        result.detected = ContentKind::Xml;
        result.output_kind = ContentKind::Xml;
        result.label = "XML";
        result.value = xml.value;
        result.structured = true;
        return result;
    }

    if (looks_like_toml(input)) return run_format_processor(input, trimmed, ProcessingMode::Toml);
    if (looks_like_ini(input)) return run_format_processor(input, trimmed, ProcessingMode::Ini);

    std::string inspected;
    if (inspect_jwt(trimmed, inspected)) {
        ProcessResult result;
        result.detected = ContentKind::Jwt;
        result.output_kind = ContentKind::Json;
        result.label = "JWT · Unverified";
        result.value = std::move(inspected);
        result.decoded = true;
        result.structured = true;
        return result;
    }

    if (looks_like_yaml(input)) {
        const auto yaml = format_yaml(input, 2);
        if (yaml.ok) {
            ProcessResult result;
            result.detected = ContentKind::Yaml;
            result.output_kind = ContentKind::Yaml;
            result.label = "YAML";
            result.value = yaml.value;
            result.structured = true;
            return result;
        }
    }

    const auto csv = parse_csv(input, '\0', true);
    if (csv.ok) {
        ProcessResult result;
        result.detected = ContentKind::Csv;
        result.output_kind = ContentKind::Csv;
        result.label = "CSV";
        result.value = csv.document.to_tsv();
        result.tabular = true;
        return result;
    }

    std::string decoded;
    std::size_t encoded_count = 0;
    if (decode_url(input, decoded, encoded_count) && encoded_count > 0 &&
        is_displayable_text(decoded)) {
        return decoded_result(ContentKind::UrlEncoded, "URL Decode", std::move(decoded));
    }

    const auto html = decode_html_entities(input);
    if (html.ok) return decoded_result(ContentKind::HtmlEntity, "HTML Entity", html.value);

    if (looks_like_hex_encoding(input)) {
        const auto hex = decode_hex(input);
        if (hex.ok && is_displayable_text(hex.value)) {
            return decoded_result(ContentKind::HexEncoded, "Hex", hex.value);
        }
    }

    decoded.clear();
    if (decode_base64(trimmed, decoded) && trimmed.size() >= 12 &&
        is_displayable_text(decoded)) {
        return decoded_result(ContentKind::Base64, "Base64", std::move(decoded));
    }

    ProcessResult result;
    result.detected = ContentKind::Text;
    result.output_kind = ContentKind::Text;
    result.label = "Text";
    result.value = input;
    return result;
}

} // namespace

const char* content_kind_name(ContentKind kind) {
    switch (kind) {
    case ContentKind::Empty: return "Empty";
    case ContentKind::Json: return "JSON";
    case ContentKind::Xml: return "XML";
    case ContentKind::Yaml: return "YAML";
    case ContentKind::Toml: return "TOML";
    case ContentKind::Ini: return "INI";
    case ContentKind::Jwt: return "JWT";
    case ContentKind::JsonEscaped: return "JSON String";
    case ContentKind::Base64: return "Base64";
    case ContentKind::UrlEncoded: return "URL Encode";
    case ContentKind::HtmlEntity: return "HTML Entity";
    case ContentKind::HexEncoded: return "Hex";
    case ContentKind::Csv: return "CSV";
    case ContentKind::Text: return "Text";
    case ContentKind::Count: return "Text";
    }
    return "Text";
}

ProcessResult process_text(const std::string& input, ProcessingMode mode) {
    const std::string trimmed = trim_ascii(input);
    if (trimmed.empty()) {
        ProcessResult result;
        result.detected = ContentKind::Empty;
        result.output_kind = ContentKind::Empty;
        result.label = "Empty";
        return result;
    }

    if (mode == ProcessingMode::Auto) {
        return detect_automatically(input, trimmed);
    }

    const auto registered = std::find_if(
        kProcessors.begin(), kProcessors.end(),
        [mode](const ProcessorRegistration& processor) {
            return processor.mode == mode;
        });
    if (registered != kProcessors.end()) {
        return registered->handler(input, trimmed, mode);
    }

    ProcessResult result = failure(
        ContentKind::Text,
        "UNREGISTERED_PROCESSOR",
        "The requested processing mode is not registered");
    result.value = input;
    return result;
}

} // namespace zeus
