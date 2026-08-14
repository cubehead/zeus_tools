#include "processing_service.h"

#include <chrono>
#include <utility>

namespace app::processing {

namespace {

zeus::ProcessingMode action_mode_for(
    int action_index,
    zeus::ContentKind detected_kind) {
    switch (action_index) {
    case 1: return zeus::ProcessingMode::JsonMinify;
    case 2: return detected_kind == zeus::ContentKind::Base64
        ? zeus::ProcessingMode::Base64
        : zeus::ProcessingMode::Base64Encode;
    case 3: return detected_kind == zeus::ContentKind::UrlEncoded
        ? zeus::ProcessingMode::UrlDecode
        : zeus::ProcessingMode::UrlEncode;
    case 4: return zeus::ProcessingMode::Csv;
    case 5: return zeus::ProcessingMode::Text;
    case 6: return zeus::ProcessingMode::JsonEscape;
    case 7: return zeus::ProcessingMode::Upper;
    case 8: return zeus::ProcessingMode::Lower;
    case 9: return zeus::ProcessingMode::Xml;
    case 10: return zeus::ProcessingMode::Yaml;
    case 11: return zeus::ProcessingMode::JsonToYaml;
    case 12: return zeus::ProcessingMode::YamlToJson;
    case 13: return zeus::ProcessingMode::JsonToXml;
    case 14: return zeus::ProcessingMode::JsonUnescape;
    case 15: return zeus::ProcessingMode::JsonToCsv;
    case 16: return zeus::ProcessingMode::XmlToJson;
    default: return zeus::ProcessingMode::Auto;
    }
}

zeus::ProcessingMode input_override_mode_for(int override_index) {
    switch (override_index) {
    case 1: return zeus::ProcessingMode::Json;
    case 2: return zeus::ProcessingMode::Xml;
    case 3: return zeus::ProcessingMode::Yaml;
    case 4: return zeus::ProcessingMode::Csv;
    case 5: return zeus::ProcessingMode::Base64;
    case 6: return zeus::ProcessingMode::UrlDecode;
    case 7: return zeus::ProcessingMode::Text;
    default: return zeus::ProcessingMode::Auto;
    }
}

zeus::ContentKind input_override_kind_for(
    int override_index,
    zeus::ContentKind detected_kind) {
    switch (override_index) {
    case 1: return zeus::ContentKind::Json;
    case 2: return zeus::ContentKind::Xml;
    case 3: return zeus::ContentKind::Yaml;
    case 4: return zeus::ContentKind::Csv;
    case 5: return zeus::ContentKind::Base64;
    case 6: return zeus::ContentKind::UrlEncoded;
    case 7: return zeus::ContentKind::Text;
    default: return detected_kind;
    }
}

char csv_delimiter_for(int index) {
    switch (index) {
    case 1: return ',';
    case 2: return '\t';
    case 3: return ';';
    case 4: return '|';
    default: return '\0';
    }
}

std::shared_ptr<zeus::HighlightedDocument> make_document(
    const zeus::ProcessResult& result,
    const std::string& display) {
    if (result.detected == zeus::ContentKind::Xml) {
        return std::make_shared<zeus::HighlightedDocument>(
            zeus::HighlightedDocument::xml(display));
    }
    if (result.detected == zeus::ContentKind::Yaml) {
        return std::make_shared<zeus::HighlightedDocument>(
            zeus::HighlightedDocument::yaml(display));
    }
    return std::make_shared<zeus::HighlightedDocument>(
        result.structured ? zeus::HighlightedDocument::json(display)
                          : zeus::HighlightedDocument::plain(display));
}

} // namespace

AnalysisResult analyze(const AnalysisRequest& request) {
    AnalysisResult output;
    output.first_row_header = request.first_row_header;
    const auto started = std::chrono::steady_clock::now();

    const zeus::ProcessResult detected = zeus::process_text(
        request.input, zeus::ProcessingMode::Auto);
    output.detected = input_override_kind_for(
        request.input_override_index, detected.detected);
    const zeus::ProcessingMode override_mode = input_override_mode_for(
        request.input_override_index);
    const zeus::ProcessResult base_result = override_mode == zeus::ProcessingMode::Auto
        ? detected : zeus::process_text(request.input, override_mode);
    const zeus::ProcessingMode action_mode = action_mode_for(
        request.action_index, output.detected);
    output.process = action_mode == zeus::ProcessingMode::Auto
        ? base_result : zeus::process_text(request.input, action_mode);

    if (output.process.tabular) {
        const std::string& table_source = output.process.value.empty()
            ? request.input : output.process.value;
        const char delimiter = request.action_index == 15
            ? ',' : csv_delimiter_for(request.csv_delimiter_index);
        const auto parsed = zeus::parse_csv(table_source, delimiter, true);
        if (parsed.ok) {
            output.csv = std::make_shared<zeus::CsvDocument>(parsed.document);
        } else {
            output.process.ok = false;
            output.process.label = "CSV";
            output.process.error_code = "CSV_PARSE_ERROR";
            output.process.error_message = parsed.error;
        }
    }

    const std::string display = output.process.value.empty()
        ? request.input : output.process.value;
    if (output.process.decoded) {
        output.decode_chain = output.process.label;
        const auto next = zeus::process_text(
            display, zeus::ProcessingMode::DecodeOneLayer);
        output.can_continue_decode = next.ok && next.decoded;
    }
    output.document = make_document(output.process, display);
    output.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started).count();
    return output;
}

DecodeResult decode_one_layer(std::string source, const std::string& previous_chain) {
    DecodeResult output;
    output.source = std::move(source);
    output.process = zeus::process_text(
        output.source, zeus::ProcessingMode::DecodeOneLayer);
    if (!output.process.ok) return output;

    const std::string display = output.process.value.empty()
        ? output.source : output.process.value;
    output.document = make_document(output.process, display);
    output.chain = previous_chain.empty()
        ? output.process.label
        : previous_chain + " → " + output.process.label;
    const auto next = zeus::process_text(display, zeus::ProcessingMode::DecodeOneLayer);
    output.can_continue = next.ok && next.decoded;
    return output;
}

} // namespace app::processing
