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

    const auto finish_csv = [&](zeus::CsvParseResult parsed,
                                zeus::ContentKind detected) {
        AnalysisResult csv_output;
        csv_output.first_row_header = request.first_row_header;
        csv_output.detected = detected;
        csv_output.process.ok = parsed.ok;
        csv_output.process.detected = detected;
        csv_output.process.output_kind = zeus::ContentKind::Csv;
        csv_output.process.label = "CSV";
        if (parsed.ok) {
            // The desktop renders CsvDocument directly. Retaining a second TSV
            // copy adds substantial latency and memory for multi-megabyte input;
            // non-presentation callers such as the CLI still receive text.
            if (!request.build_presentation) {
                csv_output.process.value = parsed.document.to_tsv();
            }
            csv_output.csv = std::make_shared<zeus::CsvDocument>(
                std::move(parsed.document));
        } else {
            csv_output.process.error_code = "CSV_PARSE_ERROR";
            csv_output.process.error_message = std::move(parsed.error);
            csv_output.process.value = request.input;
            if (request.build_presentation) {
                csv_output.document = make_document(
                    csv_output.process, csv_output.process.value);
            }
        }
        csv_output.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
        return csv_output;
    };

    const InputTypeDefinition& input_type = find_input_type(request.input_type_id);
    const bool auto_input = input_type.mode == zeus::ProcessingMode::Auto;
    const zeus::ProcessingMode override_mode = input_type.mode;

    // An explicit CSV input type, or a delimiter selected while Auto is active,
    // is enough information to parse the table once. The generic auto pipeline
    // would otherwise parse and normalize it, then this service would parse it
    // again to build the virtual table.
    if (request.action_id == "auto" &&
        (override_mode == zeus::ProcessingMode::Csv ||
         (auto_input && request.csv_delimiter_index != 0))) {
        auto parsed = zeus::parse_csv(
            request.input, csv_delimiter_for(request.csv_delimiter_index), true);
        if (parsed.ok || override_mode == zeus::ProcessingMode::Csv) {
            return finish_csv(std::move(parsed), zeus::ContentKind::Csv);
        }
    }

    zeus::ProcessResult base_result = zeus::process_text(request.input, override_mode);
    output.detected = auto_input ? base_result.detected : input_type.kind;
    const ActionDefinition* action = find_action(
        request.action_id, output.detected, request.input);
    const zeus::ProcessingMode action_mode = action
        ? action->mode : zeus::ProcessingMode::Auto;
    output.process = action_mode == zeus::ProcessingMode::Auto
        ? std::move(base_result) : zeus::process_text(request.input, action_mode);

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
        auto parsed = zeus::parse_csv(table_source, delimiter, true);
        if (parsed.ok) {
            output.process.ok = true;
            output.process.output_kind = zeus::ContentKind::Csv;
            if (!converted_to_csv) {
                output.detected = zeus::ContentKind::Csv;
                output.process.detected = zeus::ContentKind::Csv;
            }
            output.process.label = converted_to_csv ? "JSON → CSV" : "CSV";
            output.process.value = request.build_presentation
                ? std::string{} : parsed.document.to_tsv();
            output.csv = std::make_shared<zeus::CsvDocument>(std::move(parsed.document));
            output.process.error_code.clear();
            output.process.error_message.clear();
            output.process.error_line = 0;
            output.process.error_column = 0;
        } else {
            output.process.ok = false;
            output.process.output_kind = zeus::ContentKind::Csv;
            output.process.label = converted_to_csv ? "JSON → CSV" : "CSV";
            output.process.error_code = "CSV_PARSE_ERROR";
            output.process.error_message = parsed.error;
            if (!converted_to_csv) {
                output.detected = zeus::ContentKind::Csv;
                output.process.detected = zeus::ContentKind::Csv;
                output.process.value = request.input;
            }
        }
    }

    const std::string& display = output.process.value.empty()
        ? request.input : output.process.value;
    if (request.build_presentation && output.process.decoded) {
        output.decode_chain = output.process.label;
        const auto next = zeus::process_text(
            display, zeus::ProcessingMode::DecodeOneLayer);
        output.can_continue_decode = next.ok && next.decoded;
    }
    // CSV is rendered by the virtualized table view. Building a second plain
    // highlighted document for the same multi-megabyte value wastes time and
    // memory, and the desktop never displays it while the table is present.
    if (request.build_presentation && !output.csv) {
        output.document = make_document(output.process, display);
    }
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

    const std::string& display = output.process.value.empty()
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
