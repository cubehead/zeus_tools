#pragma once

#include <cstddef>
#include <memory>
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
    UnicodeDecode,
    UnicodeEncode,
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
    UnicodeEscaped,
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
    // Present only when a decoder produced non-displayable bytes. `value`
    // remains a safe text summary for UI/CLI presentation.
    std::shared_ptr<const std::string> binary_data;
    std::string error_code;
    std::string error_message;
    std::size_t error_line = 0;
    std::size_t error_column = 0;
    bool decoded = false;
};

ProcessResult process_text(const std::string& input, ProcessingMode mode = ProcessingMode::Auto);
const char* processing_mode_id(ProcessingMode mode);
const char* content_kind_name(ContentKind kind);

} // namespace zeus
