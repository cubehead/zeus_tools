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
    Base64,
    Base64Encode,
    UrlDecode,
    UrlEncode,
    Csv,
    Text,
    Upper,
    Lower,
};

enum class ContentKind {
    Empty,
    Json,
    Xml,
    Yaml,
    Jwt,
    JsonEscaped,
    Base64,
    UrlEncoded,
    Csv,
    Text,
};

struct ProcessResult {
    bool ok = true;
    ContentKind detected = ContentKind::Text;
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
