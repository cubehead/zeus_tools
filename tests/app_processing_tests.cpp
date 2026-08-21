#include "processing_service.h"
#include "processing_registry.h"
#include "large_input_paging.h"
#include "font_tokens.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <set>
#include <string>
#include <string_view>

namespace {

void expect(bool condition, std::string_view message) {
    if (condition) return;
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
}

std::string sample_for_kind(zeus::ContentKind kind) {
    switch (kind) {
    case zeus::ContentKind::Json:
        return R"({"name":"Zeus","value":1})";
    case zeus::ContentKind::Xml:
        return "<tool><name>Zeus</name><value>1</value></tool>";
    case zeus::ContentKind::Yaml:
        return "- name: Zeus\n  active: true\n- name: Tools\n  active: false";
    case zeus::ContentKind::Toml:
        return "name = \"Zeus\"\nvalue = 1";
    case zeus::ContentKind::Ini:
        return "[tool]\nname=Zeus\nvalue=1";
    case zeus::ContentKind::MongoShell:
        return R"({"count":new NumberInt("42"),"id":ObjectId("507f1f77bcf86cd799439011")})";
    case zeus::ContentKind::Jwt:
        return "eyJhbGciOiJub25lIiwidHlwIjoiSldUIn0."
               "eyJuYW1lIjoiWmV1cyIsImFjdGl2ZSI6dHJ1ZX0.signature";
    case zeus::ContentKind::JsonEscaped:
        return R"({\"name\":\"Zeus\",\"value\":1})";
    case zeus::ContentKind::Base64:
        return "SGVsbG8gWmV1cw==";
    case zeus::ContentKind::UrlEncoded:
        return "Zeus%20Tools%20%E4%BD%A0%E5%A5%BD";
    case zeus::ContentKind::HtmlEntity:
        return "Zeus &amp; &#x4F60;&#22909;";
    case zeus::ContentKind::HexEncoded:
        return "0x5a 65 75 73";
    case zeus::ContentKind::UnicodeEscaped:
        return R"(Zeus \u4F60\u597D)";
    case zeus::ContentKind::Csv:
        return "name,value\nZeus,1\nHera,2";
    case zeus::ContentKind::Text:
        return "Zeus Tools 你好";
    case zeus::ContentKind::Empty:
    case zeus::ContentKind::Count:
        return {};
    }
    return {};
}

std::string sample_for_action(const app::processing::ActionDefinition& action) {
    if (action.mode == zeus::ProcessingMode::JsonToCsv) {
        return R"([{"name":"Zeus","value":1},{"name":"Hera","value":2}])";
    }
    if (action.timestamp_candidate) return "1700000000";
    if (action.common) return sample_for_kind(zeus::ContentKind::Text);
    return sample_for_kind(action.input_kind);
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

void test_mongodb_shell_analysis() {
    app::processing::AnalysisRequest request;
    request.input = R"([{"field":"count","before":new NumberInt("421864")}])";
    const auto result = app::processing::analyze(request);

    expect(result.process.ok && result.detected == zeus::ContentKind::MongoShell,
           "MongoDB Shell input should be detected by the desktop service");
    expect(result.process.output_kind == zeus::ContentKind::Json && result.document,
           "MongoDB Shell output should use the JSON presentation model");
    expect(result.document->text().find("$numberInt") != std::string::npos &&
               !result.document->fold_regions().empty(),
           "converted Extended JSON should be highlighted, searchable and foldable");

    request.input_type_id = "mongodb";
    request.action_id = "json.minify";
    const auto minified = app::processing::analyze(request);
    expect(minified.process.ok && minified.detected == zeus::ContentKind::MongoShell &&
               minified.process.value.find('\n') == std::string::npos,
           "JSON actions should operate on converted MongoDB Shell input");

    request.action_id = "base64.encode";
    const auto encoded = app::processing::analyze(request);
    const auto original_encoded = zeus::process_text(
        request.input, zeus::ProcessingMode::Base64Encode);
    expect(encoded.process.ok && encoded.process.value == original_encoded.value,
           "common actions should continue to operate on the original MongoDB Shell text");
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

void test_binary_base64_retains_export_payload() {
    app::processing::AnalysisRequest request;
    request.input = "AAECAwQF";
    request.input_type_id = "base64";
    const auto result = app::processing::analyze(request);

    expect(result.process.ok && result.process.decoded,
           "explicit Base64 should decode binary input");
    expect(result.process.binary_data && result.process.binary_data->size() == 6,
           "desktop analysis should retain binary bytes for explicit export");
    expect(result.process.binary_extension == ".bin",
           "unknown binary data should retain a generic export extension");
    expect(result.document && result.document->text().find("Hex preview") != std::string::npos,
           "binary output should keep a searchable safe summary document");
}

void test_base64_data_url_analysis() {
    app::processing::AnalysisRequest request;
    request.input =
        "data:image/png;base64,"
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAusB9Y9Zr6sAAAAASUVORK5CYII=";
    const auto result = app::processing::analyze(request);

    expect(result.process.ok && result.process.decoded &&
               result.detected == zeus::ContentKind::Base64,
           "Base64 Data URLs should participate in automatic desktop detection");
    expect(result.process.binary_data && result.process.binary_extension == ".png",
           "Base64 Data URL analysis should preserve type-aware export bytes");
    expect(result.process.image_preview_source.rfind("data:image/png;base64,", 0) == 0,
           "desktop analysis should expose verified Data URL images for in-memory preview");
    expect(result.document && result.document->text().find("PNG image") != std::string::npos,
           "Base64 Data URL binary output should retain its safe result summary");
    expect(result.document && result.document->text().find("Dimensions: 1 × 1 px") != std::string::npos,
           "Base64 Data URL image summaries should expose decoded dimensions");
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

void test_windows_action_bar_fits_default_window() {
    constexpr float available_width = 1280.0f - 18.0f * 2.0f - 32.0f;
    constexpr bool windows = true;

    const auto width = [](float value) {
        return app::fonts::action_width_for_platform(value, windows);
    };
    const auto continues_decode = [](zeus::ContentKind kind) {
        return kind == zeus::ContentKind::Base64 ||
            kind == zeus::ContentKind::UrlEncoded ||
            kind == zeus::ContentKind::JsonEscaped ||
            kind == zeus::ContentKind::HtmlEntity ||
            kind == zeus::ContentKind::HexEncoded ||
            kind == zeus::ContentKind::UnicodeEscaped;
    };

    for (int raw_kind = 1; raw_kind < static_cast<int>(zeus::ContentKind::Count); ++raw_kind) {
        const auto kind = static_cast<zeus::ContentKind>(raw_kind);
        const std::string sample = sample_for_kind(kind);
        float total = width(116.0f) + width(64.0f);
        std::size_t items = 2;
        for (const auto& action : app::processing::registered_actions()) {
            if (!action.common && app::processing::action_applies(action, kind, sample)) {
                total += width(action.width);
                ++items;
            }
        }
        if (kind == zeus::ContentKind::Csv) {
            total += width(118.0f) + width(106.0f);
            items += 2;
        }
        if (continues_decode(kind)) {
            total += width(94.0f);
            ++items;
        }
        total += 1.0f;
        ++items;
        for (const auto& action : app::processing::registered_actions()) {
            if (action.common) {
                total += width(action.width);
                ++items;
            }
        }
        total += width(90.0f);
        ++items;
        total += static_cast<float>(items - 1) *
            app::fonts::action_gap_for_platform(windows);
        expect(total <= available_width,
               "Windows action bar should fit every content type at the default width");
    }

    expect(app::fonts::button_size_for_platform(20.0f, true) < 20.0f,
           "Windows buttons should use compact typography");
    expect(app::fonts::button_size_for_platform(20.0f, false) == 20.0f,
           "non-Windows button typography should remain unchanged");
}

void test_every_registered_input_type_executes() {
    for (const auto& type : app::processing::registered_input_types()) {
        app::processing::AnalysisRequest request;
        request.input_type_id = std::string(type.id);
        request.input = type.mode == zeus::ProcessingMode::Auto
            ? sample_for_kind(zeus::ContentKind::Json)
            : sample_for_kind(type.kind);
        const auto result = app::processing::analyze(request);
        const std::string context = "registered input type should execute: " +
            std::string(type.id);
        expect(result.process.ok, context);
        expect(result.document != nullptr || result.csv != nullptr,
               context + " should build a presentation model");
        if (type.mode != zeus::ProcessingMode::Auto) {
            expect(result.detected == type.kind,
                   context + " should retain its declared content kind");
        }
    }
}

void test_every_registered_action_executes() {
    for (const auto& action : app::processing::registered_actions()) {
        app::processing::AnalysisRequest request;
        request.input = sample_for_action(action);
        request.action_id = std::string(app::processing::action_id(action));
        const auto* applicable = app::processing::find_action(
            request.action_id, action.input_kind, request.input);
        const std::string context = "registered action should execute: " +
            request.action_id + " for " +
            std::string(zeus::content_kind_name(action.input_kind));
        expect(applicable == &action,
               context + " should resolve to its exact registry entry");
        if (action.input_kind == zeus::ContentKind::MongoShell) {
            request.input_type_id = "mongodb";
        }
        const std::string direct_input = action.input_kind == zeus::ContentKind::MongoShell &&
                action.mode != zeus::ProcessingMode::Auto
            ? zeus::process_text(request.input, zeus::ProcessingMode::MongoShell).value
            : request.input;
        const auto direct = zeus::process_text(direct_input, action.mode);
        expect(direct.ok, context + " should succeed in the core processor");
        const auto result = app::processing::analyze(request);
        expect(result.process.ok, context);
        if (!action.timestamp_candidate) {
            expect(result.detected == action.input_kind,
                   context + " should be applied to the intended detected type");
        }
        expect(result.process.output_kind == direct.output_kind,
               context + " should retain the handler output type");
        expect(result.document != nullptr || result.csv != nullptr,
               context + " should build a presentation model");
    }
}

void test_all_processors_reject_adversarial_input_without_throwing() {
    std::string deeply_nested(256, '[');
    deeply_nested.append(256, ']');
    const std::string invalid_utf8("\xFF\xFE\xC0", 3);
    const std::string samples[] = {
        invalid_utf8,
        R"({"broken":[1,})",
        R"(<!DOCTYPE tool [<!ENTITY xxe SYSTEM "file:///etc/passwd">]><tool>&xxe;</tool>)",
        "name,quote\nZeus,\"unterminated",
        R"(\uD800 broken surrogate %ZZ ====)",
        deeply_nested,
    };

    for (int index = 0; index < static_cast<int>(zeus::ProcessingMode::Count); ++index) {
        const auto mode = static_cast<zeus::ProcessingMode>(index);
        for (const auto& sample : samples) {
            try {
                (void)zeus::process_text(sample, mode);
            } catch (const std::exception& exception) {
                std::cerr << "FAILED: processor threw for mode "
                          << zeus::processing_mode_id(mode) << ": "
                          << exception.what() << '\n';
                std::exit(1);
            } catch (...) {
                std::cerr << "FAILED: processor threw an unknown exception for mode "
                          << zeus::processing_mode_id(mode) << '\n';
                std::exit(1);
            }
        }
    }

    for (const auto& type : app::processing::registered_input_types()) {
        for (const auto& sample : samples) {
            app::processing::AnalysisRequest request;
            request.input = sample;
            request.input_type_id = std::string(type.id);
            try {
                (void)app::processing::analyze(request);
            } catch (const std::exception& exception) {
                std::cerr << "FAILED: processing service threw for input type "
                          << type.id << ": " << exception.what() << '\n';
                std::exit(1);
            } catch (...) {
                std::cerr << "FAILED: processing service threw an unknown exception for input type "
                          << type.id << '\n';
                std::exit(1);
            }
        }
    }
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
    test_mongodb_shell_analysis();
    test_decode_one_layer();
    test_binary_base64_retains_export_payload();
    test_base64_data_url_analysis();
    test_recommended_input_size_boundary();
    test_large_input_pages_preserve_utf8_boundaries();
    test_processing_registry();
    test_windows_action_bar_fits_default_window();
    test_every_registered_input_type_executes();
    test_every_registered_action_executes();
    test_all_processors_reject_adversarial_input_without_throwing();
    std::cout << "All app processing tests passed.\n";
    return 0;
}
