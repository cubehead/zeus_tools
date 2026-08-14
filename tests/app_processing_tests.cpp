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

void test_input_override() {
    app::processing::AnalysisRequest request;
    request.input = "name: Zeus\nenabled: true";
    request.input_override_index = 3;
    const auto result = app::processing::analyze(request);

    expect(result.process.ok, "YAML override should succeed");
    expect(result.detected == zeus::ContentKind::Yaml,
           "input override should control the detected kind");
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
    test_input_override();
    test_decode_one_layer();
    std::cout << "All app processing tests passed.\n";
    return 0;
}
