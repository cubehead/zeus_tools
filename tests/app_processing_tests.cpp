#include "processing_service.h"

#include <cstdlib>
#include <iostream>
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
    expect(result.process.structured, "JSON result should be structured");
    expect(result.document != nullptr, "JSON should create a highlighted document");
    expect(result.document->text() == result.process.value,
           "highlighted JSON should match the processed value");
}

void test_csv_analysis() {
    app::processing::AnalysisRequest request;
    request.input = "name,value\nZeus,1\nHera,2";
    request.action_index = 4;
    const auto result = app::processing::analyze(request);

    expect(result.process.ok, "CSV analysis should succeed");
    expect(result.csv != nullptr, "CSV analysis should create a table document");
    expect(result.csv->rows.size() == 3, "CSV should retain all rows");
    expect(result.first_row_header, "CSV header preference should be retained");
}

void test_csv_manual_delimiters_use_original_input() {
    app::processing::AnalysisRequest comma_request;
    comma_request.input = "name,value\nZeus,1\nHera,2";
    comma_request.action_index = 4;
    comma_request.csv_delimiter_index = 1;
    const auto comma = app::processing::analyze(comma_request);
    expect(comma.process.ok && comma.csv != nullptr,
           "manual comma CSV should parse the original input");
    expect(comma.csv->delimiter == ',' && comma.csv->rows[1][1] == "1",
           "manual comma CSV should retain comma-separated cells");

    app::processing::AnalysisRequest semicolon_request;
    semicolon_request.input = "name;value\nZeus;1\nHera;2";
    semicolon_request.input_override_index = 6;
    semicolon_request.csv_delimiter_index = 3;
    const auto semicolon = app::processing::analyze(semicolon_request);
    expect(semicolon.process.ok && semicolon.csv != nullptr,
           "CSV override should honor a manually selected delimiter");
    expect(semicolon.csv->delimiter == ';' && semicolon.csv->rows[2][0] == "Hera",
           "manual semicolon CSV should retain semicolon-separated cells");
}

void test_input_override() {
    app::processing::AnalysisRequest request;
    request.input = "name: Zeus\nenabled: true";
    request.input_override_index = 3;
    const auto result = app::processing::analyze(request);

    expect(result.process.ok, "YAML override should succeed");
    expect(result.detected == zeus::ContentKind::Yaml,
           "input override should control the detected kind");
}

void test_toml_analysis() {
    app::processing::AnalysisRequest request;
    request.input = "title = \"Zeus\"\n[window]\nwidth = 1200";
    request.input_override_index = 4;
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
    request.input_override_index = 5;
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
    expect(result.document != nullptr, "decoded JSON should create a document");
    expect(result.document->text().find("\"name\": \"Zeus\"") != std::string::npos,
           "decoded JSON should be formatted");
}

} // namespace

int main() {
    test_json_analysis();
    test_csv_analysis();
    test_csv_manual_delimiters_use_original_input();
    test_input_override();
    test_toml_analysis();
    test_ini_analysis();
    test_decode_one_layer();
    std::cout << "All app processing tests passed.\n";
    return 0;
}
