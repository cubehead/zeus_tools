#include "processing_service.h"
#include "processing_registry.h"
#include "large_input_paging.h"

#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

namespace {

void expect(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
}

void test_json_analysis() {
    app::processing::AnalysisRequest request;
    request.input = R"({"name":"Zeus","enabled":true})";
    const auto result = app::processing::analyze(request);

    expect(result.process.ok, "JSON analysis should succeed");
    expect(result.detected == zeus::ContentKind::Json, "JSON should be detected");
    expect(result.process.output_kind == zeus::ContentKind::Json,
           "JSON result should expose JSON as its output kind");
    expect(result.document != nullptr, "JSON should create a highlighted document");
    expect(result.document->text() == result.process.value,
           "highlighted JSON should match the processed value");
}

void test_csv_analysis() {
    app::processing::AnalysisRequest request;
    request.input = "name,value\nZeus,1\nHera,2";
    request.input_type_id = "csv";
    const auto result = app::processing::analyze(request);

    expect(result.process.ok, "CSV analysis should succeed");
    expect(result.csv != nullptr, "CSV analysis should create a table document");
    expect(result.document == nullptr,
           "CSV analysis should not duplicate the table as a highlighted document");
    expect(result.csv->rows.size() == 3, "CSV should retain all rows");
    expect(result.first_row_header, "CSV header preference should be retained");
}

void test_json_to_csv_retains_source_kind() {
    app::processing::AnalysisRequest request;
    request.input = R"([{"name":"Zeus","value":1},{"name":"Hera","value":2}])";
    request.action_id = "json.to_csv";
    const auto result = app::processing::analyze(request);

    expect(result.process.ok && result.csv != nullptr,
           "JSON to CSV should create a table document");
    expect(result.detected == zeus::ContentKind::Json &&
               result.process.detected == zeus::ContentKind::Json,
           "JSON to CSV should retain JSON as its source kind");
    expect(result.process.output_kind == zeus::ContentKind::Csv,
           "JSON to CSV should expose CSV as its output kind");
}

void test_csv_manual_delimiters_use_original_input() {
    app::processing::AnalysisRequest comma_request;
    comma_request.input = "name,value\nZeus,1\nHera,2";
    comma_request.input_type_id = "csv";
    comma_request.csv_delimiter_index = 1;
    const auto comma = app::processing::analyze(comma_request);
    expect(comma.process.ok && comma.csv != nullptr,
           "manual comma CSV should parse the original input");
    expect(comma.csv->delimiter == ',' && comma.csv->rows[1][1] == "1",
           "manual comma CSV should retain comma-separated cells");

    app::processing::AnalysisRequest semicolon_request;
    semicolon_request.input = "name;value\nZeus;1\nHera;2";
    semicolon_request.input_type_id = "csv";
    semicolon_request.csv_delimiter_index = 3;
    const auto semicolon = app::processing::analyze(semicolon_request);
    expect(semicolon.process.ok && semicolon.csv != nullptr,
           "CSV override should honor a manually selected delimiter");
    expect(semicolon.csv->delimiter == ';' && semicolon.csv->rows[2][0] == "Hera",
           "manual semicolon CSV should retain semicolon-separated cells");
}

void test_auto_csv_uses_manual_delimiter_hint() {
    app::processing::AnalysisRequest request;
    request.input = "name;value\nZeus;1\nHera;2";
    request.csv_delimiter_index = 3;
    const auto result = app::processing::analyze(request);

    expect(result.process.ok && result.csv != nullptr,
           "Auto input should use an explicitly selected CSV delimiter");
    expect(result.detected == zeus::ContentKind::Csv &&
               result.csv->delimiter == ';',
           "delimiter-guided Auto analysis should retain CSV detection");
    expect(result.process.value.empty(),
           "desktop CSV analysis should not retain a duplicate TSV value");
}

void test_invalid_explicit_csv_retains_error_presentation() {
    app::processing::AnalysisRequest request;
    request.input = "name,value\nZeus";
    request.input_type_id = "csv";
    request.csv_delimiter_index = 1;
    const auto result = app::processing::analyze(request);

    expect(!result.process.ok && result.csv == nullptr,
           "invalid explicit CSV should retain a failed table result");
    expect(result.document != nullptr &&
               result.document->text() == request.input,
           "invalid explicit CSV should retain source text for error presentation");
}

void test_input_override() {
    app::processing::AnalysisRequest request;
    request.input = "name: Zeus\nenabled: true";
    request.input_type_id = "yaml";
    const auto result = app::processing::analyze(request);

    expect(result.process.ok, "YAML override should succeed");
    expect(result.detected == zeus::ContentKind::Yaml,
           "input override should control the detected kind");
}

void test_text_override_wins_over_auto_detection() {
    app::processing::AnalysisRequest request;
    request.input = R"({"name":"Zeus","enabled":true})";
    request.input_type_id = "text";
    const auto result = app::processing::analyze(request);

    expect(result.process.ok, "text override should succeed for JSON-looking input");
    expect(result.detected == zeus::ContentKind::Text,
           "manual text input should not inherit automatic JSON detection");
    expect(result.process.output_kind == zeus::ContentKind::Text,
           "manual text input should retain plain-text presentation");
    expect(result.process.value == request.input,
           "manual text input should preserve its original value");
}

void test_toml_analysis() {
    app::processing::AnalysisRequest request;
    request.input = "title = \"Zeus\"\n[window]\nwidth = 1200";
    request.input_type_id = "toml";
    const auto result = app::processing::analyze(request);

    expect(result.process.ok, "TOML override should succeed");
    expect(result.detected == zeus::ContentKind::Toml,
           "TOML override should control the detected kind");
    expect(result.document != nullptr && !result.document->lines().empty(),
           "TOML should create a highlighted document");
}

void test_ini_analysis() {
    app::processing::AnalysisRequest request;
    request.input = "[window]\nwidth=1200\ntheme=system";
    request.input_type_id = "ini";
    const auto result = app::processing::analyze(request);

    expect(result.process.ok, "INI override should succeed");
    expect(result.detected == zeus::ContentKind::Ini,
           "INI override should control the detected kind");
}

void test_decode_one_layer() {
    const std::string encoded = "eyJuYW1lIjoiWmV1cyJ9";
    const auto result = app::processing::decode_one_layer(encoded, {});

    expect(result.process.ok && result.process.decoded,
           "Base64 should decode by one layer");
    expect(result.process.detected == zeus::ContentKind::Base64,
           "decode result should retain its source kind");
    expect(result.process.output_kind == zeus::ContentKind::Json,
           "decode result should expose JSON as its output kind");
    expect(result.document != nullptr, "decoded JSON should create a document");
    expect(result.document->text().find("\"name\": \"Zeus\"") != std::string::npos,
           "decoded JSON should be formatted");
}

void test_recommended_input_size_boundary() {
    expect(!app::processing::exceeds_recommended_input_size(
               app::processing::kRecommendedMaxInputBytes),
           "exactly 10 MiB should remain within the recommended desktop limit");
    expect(app::processing::exceeds_recommended_input_size(
               app::processing::kRecommendedMaxInputBytes + 1),
           "input above 10 MiB should require explicit desktop approval");
    expect(!app::processing::requires_lightweight_input_preview(
               app::processing::kInteractiveEditorMaxBytes),
           "exactly 1 MiB should remain in the interactive input editor");
    expect(app::processing::requires_lightweight_input_preview(
               app::processing::kInteractiveEditorMaxBytes + 1),
           "input above 1 MiB should use the lightweight input preview");
}

void test_large_input_pages_preserve_utf8_boundaries() {
    std::string input(app::large_input::kPageBytes - 1, 'x');
    input += "你好";
    input.append(app::large_input::kPageBytes, 'y');
    expect(app::large_input::page_count(input.size()) == 3,
           "large input should expose deterministic fixed-size pages");
    const auto first = app::large_input::page_range(input, 0);
    const auto second = app::large_input::page_range(input, 1);
    const auto third = app::large_input::page_range(input, 2);
    const auto boundaries = app::large_input::page_boundaries(input);
    expect(first.start == 0 && first.end == second.start,
           "adjacent large-input pages should not overlap or leave gaps");
    expect(second.end == third.start && third.end == input.size(),
           "all large-input page ranges should cover the complete source");
    expect((static_cast<unsigned char>(input[second.start]) & 0xC0U) != 0x80U,
           "large-input pages should start on a UTF-8 code point boundary");
    expect(boundaries.size() == 4 && boundaries[1] == first.end &&
               boundaries[2] == second.end && boundaries[3] == input.size(),
           "stored page boundaries should match direct UTF-8-safe page ranges");
    auto resized = boundaries;
    const std::size_t original_total = resized.back();
    const std::size_t original_first = resized[1] - resized[0];
    app::large_input::resize_page(resized, 0, original_first + 7);
    expect(resized[0] == 0 && resized[1] == boundaries[1] + 7 &&
               resized.back() == original_total + 7,
           "growing a page should shift every following boundary");
    app::large_input::resize_page(resized, 0, original_first);
    expect(resized == boundaries,
           "shrinking a page should restore all following boundaries");
}

void test_processing_registry() {
    for (int index = 0; index < static_cast<int>(zeus::ContentKind::Count); ++index) {
        const auto kind = static_cast<zeus::ContentKind>(index);
        expect(app::processing::content_definition(kind).kind == kind,
               "every content kind should resolve to its registered metadata");
    }
    expect(app::processing::content_definition(zeus::ContentKind::Toml).export_extension ==
               ".toml",
           "TOML metadata should provide its export extension");
    expect(app::processing::content_definition(zeus::ContentKind::Ini).syntax ==
               app::processing::DocumentSyntax::Toml,
           "INI metadata should select the key/value highlighter");
    expect(app::processing::content_definition(
               zeus::ContentKind::UnicodeEscaped).compact_label == "Unicode",
           "Unicode escape metadata should provide a compact action-bar label");

    std::set<std::string> action_keys;
    for (const auto& action : app::processing::registered_actions()) {
        const std::string_view id = app::processing::action_id(action);
        expect(!id.empty(), "every registered action should resolve a stable core ID");
        expect(id == zeus::processing_mode_id(action.mode),
               "application actions should derive IDs from their processing modes");
        const std::string key = std::string(id) + ":" +
            (action.common ? "common" : std::to_string(static_cast<int>(action.input_kind)));
        expect(action_keys.insert(key).second,
               "registered action IDs should be unique within their applicability scope");
    }

    std::set<std::string> input_ids;
    for (const auto& type : app::processing::registered_input_types()) {
        expect(input_ids.insert(std::string(type.id)).second,
               "registered input type IDs should be unique");
    }

    const auto* encode = app::processing::find_action(
        "base64.encode", zeus::ContentKind::Base64, "SGVsbG8=");
    expect(encode != nullptr && encode->mode == zeus::ProcessingMode::Base64Encode,
           "Base64 encode should remain an encode action for encoded input");
    const auto* unicode_encode = app::processing::find_action(
        "unicode.escape", zeus::ContentKind::Text, "你好");
    expect(unicode_encode != nullptr &&
               unicode_encode->mode == zeus::ProcessingMode::UnicodeEncode,
           "Unicode escape should be registered as a common text action");
    expect(app::processing::find_input_type("unicode").mode ==
               zeus::ProcessingMode::UnicodeDecode,
           "Unicode escaped text should support an explicit input override");

    bool found_toml = false;
    for (const auto& action : app::processing::registered_actions()) {
        if (app::processing::action_id(action) == "json.to_toml" &&
            app::processing::action_applies(
                action, zeus::ContentKind::Json, R"({"name":"Zeus"})")) {
            found_toml = true;
        }
    }
    expect(found_toml, "JSON registry should expose its TOML conversion");
    expect(app::processing::input_type_index("toml") == 4,
           "input type lookup should use stable IDs");
    expect(app::processing::find_input_type("missing").id == "auto",
           "unknown input types should safely fall back to auto");
}

} // namespace

int main() {
    test_json_analysis();
    test_csv_analysis();
    test_json_to_csv_retains_source_kind();
    test_csv_manual_delimiters_use_original_input();
    test_auto_csv_uses_manual_delimiter_hint();
    test_invalid_explicit_csv_retains_error_presentation();
    test_input_override();
    test_text_override_wins_over_auto_detection();
    test_toml_analysis();
    test_ini_analysis();
    test_decode_one_layer();
    test_recommended_input_size_boundary();
    test_large_input_pages_preserve_utf8_boundaries();
    test_processing_registry();
    std::cout << "All app processing tests passed.\n";
    return 0;
}
