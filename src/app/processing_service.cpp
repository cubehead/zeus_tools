#include "processing_service.h"
#include "processing_registry.h"

#include <chrono>
#include <utility>

namespace app::processing {

namespace {

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
    const DocumentSyntax syntax = content_definition(result.output_kind).syntax;
    if (syntax == DocumentSyntax::Xml) {
        return std::make_shared<zeus::HighlightedDocument>(
            zeus::HighlightedDocument::xml(display));
    }
    if (syntax == DocumentSyntax::Yaml) {
        return std::make_shared<zeus::HighlightedDocument>(
            zeus::HighlightedDocument::yaml(display));
    }
    if (syntax == DocumentSyntax::Toml) {
        return std::make_shared<zeus::HighlightedDocument>(
            zeus::HighlightedDocument::toml(display));
    }
    return std::make_shared<zeus::HighlightedDocument>(
        syntax == DocumentSyntax::Json
            ? zeus::HighlightedDocument::json(display)
            : zeus::HighlightedDocument::plain(display));
}

} // namespace

AnalysisResult analyze(const AnalysisRequest& request) {
    AnalysisResult output;
    output.first_row_header = request.first_row_header;
    const auto started = std::chrono::steady_clock::now();

    const zeus::ProcessResult detected = zeus::process_text(
        request.input, zeus::ProcessingMode::Auto);
    const InputTypeDefinition& input_type = find_input_type(request.input_type_id);
    const bool auto_input = input_type.mode == zeus::ProcessingMode::Auto;
    output.detected = auto_input ? detected.detected : input_type.kind;
    const zeus::ProcessingMode override_mode = input_type.mode;
    const zeus::ProcessResult base_result = override_mode == zeus::ProcessingMode::Auto
        ? detected : zeus::process_text(request.input, override_mode);
    const ActionDefinition* action = find_action(
        request.action_id, output.detected, request.input);
    const zeus::ProcessingMode action_mode = action
        ? action->mode : zeus::ProcessingMode::Auto;
    output.process = action_mode == zeus::ProcessingMode::Auto
        ? base_result : zeus::process_text(request.input, action_mode);

    const bool csv_requested = override_mode == zeus::ProcessingMode::Csv ||
        action_mode == zeus::ProcessingMode::Csv;
    const bool converted_to_csv = action_mode == zeus::ProcessingMode::JsonToCsv;
    if (csv_requested || output.process.output_kind == zeus::ContentKind::Csv) {
        // Parse user CSV from the original text. process_text normalizes a
        // detected CSV to TSV, so parsing output.process.value with the user's
        // comma/semicolon choice would incorrectly reject a valid table.
        const std::string& table_source = converted_to_csv
            ? output.process.value : request.input;
        const char delimiter = converted_to_csv
            ? ',' : csv_delimiter_for(request.csv_delimiter_index);
        const auto parsed = zeus::parse_csv(table_source, delimiter, true);
        if (parsed.ok) {
            output.csv = std::make_shared<zeus::CsvDocument>(parsed.document);
            output.detected = zeus::ContentKind::Csv;
            output.process.ok = true;
            output.process.detected = zeus::ContentKind::Csv;
            output.process.output_kind = zeus::ContentKind::Csv;
            output.process.label = converted_to_csv ? "JSON → CSV" : "CSV";
            output.process.value = parsed.document.to_tsv();
            output.process.error_code.clear();
            output.process.error_message.clear();
            output.process.error_line = 0;
            output.process.error_column = 0;
        } else {
            output.process.ok = false;
            output.process.detected = zeus::ContentKind::Csv;
            output.process.output_kind = zeus::ContentKind::Csv;
            output.process.label = "CSV";
            output.process.error_code = "CSV_PARSE_ERROR";
            output.process.error_message = parsed.error;
            output.process.value = request.input;
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
