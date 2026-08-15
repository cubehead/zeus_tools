#pragma once

#include <cstddef>
#include <string>

namespace zeus {

enum class ProcessingMode {
    Auto,
    Json,
    JsonMinify,
    JsonEscape,
    JsonUnescape,
    JsonToYaml,
    JsonToXml,
    JsonToCsv,
    Xml,
    XmlToJson,
    Yaml,
    YamlToJson,
    Toml,
    TomlToJson,
    JsonToToml,
    Ini,
    IniToJson,
    Base64,
    Base64Encode,
    UrlDecode,
    UrlEncode,
    DecodeOneLayer,
    Csv,
    Text,
    Upper,
    Lower,
    HtmlEntityDecode,
    HtmlEntityEncode,
    HexDecode,
    HexEncode,
    Timestamp,
    Count,
};

enum class ContentKind {
    Empty,
    Json,
    Xml,
    Yaml,
    Toml,
    Ini,
    Jwt,
    JsonEscaped,
    Base64,
    UrlEncoded,
    HtmlEntity,
    HexEncoded,
    Csv,
    Text,
    Count,
};

struct ProcessResult {
    bool ok = true;
    // `detected` describes the source/input; `output_kind` describes `value`.
    ContentKind detected = ContentKind::Text;
    ContentKind output_kind = ContentKind::Text;
    std::string label = "Text";
    std::string value;
    std::string error_code;
    std::string error_message;
    std::size_t error_line = 0;
    std::size_t error_column = 0;
    bool decoded = false;
    bool structured = false;
    bool tabular = false;
};

ProcessResult process_text(const std::string& input, ProcessingMode mode = ProcessingMode::Auto);
const char* content_kind_name(ContentKind kind);

} // namespace zeus
