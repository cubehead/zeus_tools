#include "zeus/json_formatter.h"
#include "zeus/crypto_service.h"
#include "zeus/csv_document.h"
#include "zeus/text_document.h"
#include "zeus/text_selection.h"
#include "zeus/text_processor.h"
#include "zeus/structured_converter.h"
#include "zeus/xml_formatter.h"
#include "zeus/yaml_formatter.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    require(zeus::compute_digest("", zeus::DigestAlgorithm::Md5).hex ==
                "d41d8cd98f00b204e9800998ecf8427e",
            "MD5 should match its standard empty-string vector");
    require(zeus::compute_digest("abc", zeus::DigestAlgorithm::Sha1).hex ==
                "a9993e364706816aba3e25717850c26c9cd0d89d",
            "SHA-1 should match its standard abc vector");
    const auto sha256 = zeus::compute_digest("abc", zeus::DigestAlgorithm::Sha256);
    require(sha256.hex ==
                "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            "SHA-256 should match its standard abc vector");
    require(sha256.base64 == "ungWv48Bz+pBQUDeXa4iI7ADYaOWF3qctBD/YfIAFa0=",
            "digest Base64 output should match the raw digest bytes");
    require(zeus::compute_digest("abc", zeus::DigestAlgorithm::Sha512).hex ==
                "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f",
            "SHA-512 should match its standard abc vector");

    const std::string hmac_message = "The quick brown fox jumps over the lazy dog";
    require(zeus::compute_hmac(hmac_message, "key", zeus::DigestAlgorithm::Md5).hex ==
                "80070713463e7749b90c2dc24911e275",
            "HMAC-MD5 should match the standard vector");
    require(zeus::compute_hmac(hmac_message, "key", zeus::DigestAlgorithm::Sha1).hex ==
                "de7c9b85b8b78aa6bc8a7a36f70a90701c9db4d9",
            "HMAC-SHA1 should match the standard vector");
    require(zeus::compute_hmac(hmac_message, "key", zeus::DigestAlgorithm::Sha256).hex ==
                "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8",
            "HMAC-SHA256 should match the standard vector");
    require(zeus::compute_hmac(hmac_message, "key", zeus::DigestAlgorithm::Sha512).hex ==
                "b42af09057bac1e2d41708e48a902e09b5ff7f12ab428a4fe86653c73dd248fb"
                "82f948a549f7b791a5b41915ee4d1ec3935357e4e2317250d0372afa2ebeeb3a",
            "HMAC-SHA512 should match the standard vector");
    require(zeus::digest_algorithm_is_weak(zeus::DigestAlgorithm::Md5) &&
                zeus::digest_algorithm_is_weak(zeus::DigestAlgorithm::Sha1) &&
                !zeus::digest_algorithm_is_weak(zeus::DigestAlgorithm::Sha256),
            "MD5 and SHA-1 should be marked as weak algorithms");

    const auto formatted = zeus::format_json(R"({"name":"Zeus","active":true,"count":3,"items":[null,2]})");
    require(formatted.ok, "valid JSON should format");
    require(formatted.value.find("\n  \"name\": \"Zeus\"") != std::string::npos, "object should be indented");
    require(formatted.value.find("\n    null") != std::string::npos, "array should be indented");

    const auto invalid = zeus::format_json("{\n  \"name\": 01\n}");
    require(!invalid.ok, "leading zero should fail");
    require(invalid.issue.line == 2, "error line should be reported");
    require(!invalid.issue.code.empty(), "error code should be stable");

    const auto escaped = zeus::format_json(R"({"unicode":"\u4f60\u597d","quote":"\""})");
    require(escaped.ok, "valid escapes should format");

    const auto document = zeus::HighlightedDocument::json(formatted.value);
    require(document.lines().size() > 4, "formatted document should expose lines");
    require(!document.lines()[1].spans.empty(), "JSON lines should expose highlight spans");
    require(!document.fold_regions().empty() &&
                document.fold_regions().front().start_line == 0 &&
                document.fold_regions().front().end_line == document.lines().size() - 1,
            "formatted JSON should expose a fold region for the root container");
    zeus::TextFoldState folds;
    folds.ensure_document(document);
    require(folds.toggle(document, 0), "a JSON container line should toggle folding");
    require(folds.visible_lines(document).size() == 1,
            "folding the JSON root should hide all child lines");
    require(folds.reveal(document, 2) && !folds.is_collapsed(0),
            "revealing a hidden search result should expand its containing JSON region");

    const auto matches = document.search("zeus");
    require(matches.size() == 1, "case-insensitive search should find result");
    require(matches.front().line == 1, "search should report zero-based line");
    require(matches.front().length == 4 &&
                document.lines()[matches.front().line].text.substr(
                    matches.front().column, matches.front().length) == "Zeus",
            "search should retain the exact byte range for inline highlighting");

    std::string regex_error;
    const auto regex_matches = document.search("(true|false)", true, true, &regex_error);
    require(regex_error.empty(), "valid regex should not error");
    require(regex_matches.size() == 1, "regex should find boolean");

    document.search("(", true, true, &regex_error);
    require(!regex_error.empty(), "invalid regex should return an error");

    zeus::TextSelection selection;
    selection.begin(document, 1, 2);
    selection.extend(document, 2, 5);
    require(!selection.empty(), "dragging across lines should create a selection");
    require(selection.selected_text(document).find("\n") != std::string::npos,
            "cross-line selection should preserve newlines");
    const auto first_line_columns = selection.columns_for_line(document, 1);
    require(first_line_columns.second > first_line_columns.first,
            "selection should expose the visible byte range for a line");

    selection.select_all(document);
    require(selection.selected_text(document) == document.text(), "select all should cover the result");

    selection.select_line(document, 1);
    require(selection.selected_text(document).back() == '\n',
            "selecting a non-final line should include its newline");
    require(selection.includes_line_break_after(document, 1),
            "selection model should expose selected line breaks for continuous highlighting");

    selection.select_lines(document, 4, 1);
    require(selection.selected_text(document).find("\n") != std::string::npos,
            "reverse line-gutter selection should preserve the whole line range");
    require(selection.selected_length() == selection.selected_text(document).size(),
            "selection length should be available without materializing another text copy");

    require(zeus::selection_line_at_viewport_position(1000, 25.0f, 5000.0f, 125.0f) == 205,
            "drag selection should resolve lines from absolute scroll position");
    require(zeus::selection_line_at_viewport_position(1000, 25.0f, 5000.0f, 50000.0f) == 999,
            "drag selection line resolution should clamp to the document end");

    std::string large_text;
    large_text.reserve(2 * 1024 * 1024);
    while (large_text.size() < 2 * 1024 * 1024) {
        large_text += "0123456789abcdef\n";
    }
    const auto large_document = zeus::HighlightedDocument::plain(std::move(large_text));
    selection.select_all(large_document);
    require(selection.selected_length() == large_document.text().size(),
            "large selections should report their byte count without copying selected text");

    const auto word_document = zeus::HighlightedDocument::plain("alpha beta 中文测试,done");
    selection.select_word(word_document, 0, 7);
    require(selection.selected_text(word_document) == "beta",
            "double-click selection should select an ASCII word");
    const std::size_t chinese_offset = word_document.text().find("中文");
    selection.select_word(word_document, 0, chinese_offset);
    require(selection.selected_text(word_document) == "中文测试",
            "double-click selection should select contiguous UTF-8 text");
    const std::size_t comma_offset = word_document.text().find(',');
    selection.select_word(word_document, 0, comma_offset);
    require(selection.selected_text(word_document) == ",",
            "double-click selection should select only one punctuation character");

    const auto json_word_document = zeus::HighlightedDocument::plain("{\"name\":\"Zeus\"}");
    selection.select_word(json_word_document, 0, 3);
    require(selection.selected_text(json_word_document) == "name",
            "double-click selection should exclude JSON quotes and separators");
    const auto cjk_punctuation_document = zeus::HighlightedDocument::plain("中文，测试");
    selection.select_word(cjk_punctuation_document, 0, 0);
    require(selection.selected_text(cjk_punctuation_document) == "中文",
            "double-click selection should exclude full-width punctuation");
    selection.select_word(cjk_punctuation_document, 0, std::string("中文").size());
    require(selection.selected_text(cjk_punctuation_document) == "，",
            "double-click selection should isolate full-width punctuation");

    const auto unicode_document = zeus::HighlightedDocument::json("{\n  \"value\": \"你好\"\n}");
    selection.begin(unicode_document, 1, 14);
    selection.extend(unicode_document, 1, 16);
    const auto unicode_range = selection.range();
    require((static_cast<unsigned char>(unicode_document.text()[unicode_range.first]) & 0xC0U) != 0x80U,
            "selection offsets must clamp to UTF-8 boundaries");

    const auto detected_json = zeus::process_text(R"({"answer":42})");
    require(detected_json.ok && detected_json.detected == zeus::ContentKind::Json,
            "auto mode should detect JSON");
    require(detected_json.structured, "detected JSON should be marked structured");

    const auto escaped_json_string = zeus::process_text(R"("{\"name\":\"Zeus\",\"enabled\":true}")");
    require(escaped_json_string.ok && escaped_json_string.detected == zeus::ContentKind::JsonEscaped &&
                escaped_json_string.decoded &&
                escaped_json_string.value.find("\"name\": \"Zeus\"") != std::string::npos,
            "a JSON string containing JSON should unescape exactly one layer and format the result");
    const auto raw_escaped_json = zeus::process_text(R"({\"name\":\"Zeus\"})");
    require(raw_escaped_json.ok && raw_escaped_json.detected == zeus::ContentKind::JsonEscaped &&
                raw_escaped_json.value.find("\"name\": \"Zeus\"") != std::string::npos,
            "raw escaped JSON content should be detected and unescaped");
    const auto ordinary_json_string = zeus::process_text(R"("hello world")");
    require(ordinary_json_string.detected == zeus::ContentKind::Json,
            "ordinary JSON strings must not be misdetected as embedded JSON");
    const auto one_layer_escaped_json = zeus::process_text(
        R"("\"{\\\"name\\\":\\\"Zeus\\\"}\"")");
    require(one_layer_escaped_json.detected == zeus::ContentKind::JsonEscaped &&
                one_layer_escaped_json.value.find("\\\"name\\\"") != std::string::npos,
            "escaped JSON inspection must stop after one layer");

    const auto formatted_xml = zeus::format_xml(
        "<?xml version=\"1.0\"?><root id=\"7\"><item>Zeus</item><empty/></root>");
    require(formatted_xml.ok && formatted_xml.value.find("\n  <item>Zeus</item>") != std::string::npos,
            "valid XML should format element-only children without expanding mixed text");
    const auto mixed_xml = zeus::format_xml("<p>Hello <b>Zeus</b>!</p>");
    require(mixed_xml.ok && mixed_xml.value == "<p>Hello <b>Zeus</b>!</p>",
            "XML formatting must not insert semantic whitespace into mixed content");
    const auto invalid_xml = zeus::format_xml("<root>\n  <item></root>");
    require(!invalid_xml.ok && invalid_xml.issue.line == 2,
            "invalid XML should report parser coordinates");
    const auto unsafe_xml = zeus::format_xml("<!DOCTYPE root [<!ENTITY x SYSTEM \"file:///etc/passwd\">]><root>&x;</root>");
    require(!unsafe_xml.ok && unsafe_xml.issue.code == "XML_UNSAFE_DECLARATION",
            "XML parser must reject DTD and entity declarations before parsing");
    const auto detected_xml = zeus::process_text("<root><item value=\"1\" /></root>");
    require(detected_xml.ok && detected_xml.detected == zeus::ContentKind::Xml && detected_xml.structured,
            "auto mode should detect and format XML");
    const auto xml_document = zeus::HighlightedDocument::xml(detected_xml.value);
    require(!xml_document.lines().empty() && !xml_document.lines().front().spans.empty(),
            "formatted XML should expose syntax highlight spans");
    require(!xml_document.fold_regions().empty(),
            "multi-line formatted XML elements should expose fold regions");

    const auto formatted_yaml = zeus::format_yaml(
        "name: Zeus Tools\nenabled: true\nitems:\n  - json\n  - xml\n");
    require(formatted_yaml.ok && formatted_yaml.value.find("name: Zeus Tools") != std::string::npos,
            "common single-document YAML should parse and normalize");
    const auto detected_yaml = zeus::process_text(
        "name: Zeus Tools\nenabled: true\nitems:\n  - json\n  - xml\n");
    require(detected_yaml.ok && detected_yaml.detected == zeus::ContentKind::Yaml && detected_yaml.structured,
            "auto mode should conservatively detect multi-line YAML mappings");
    const auto yaml_document = zeus::HighlightedDocument::yaml(detected_yaml.value);
    require(!yaml_document.lines().empty() && !yaml_document.lines().front().spans.empty(),
            "formatted YAML should expose syntax highlight spans");
    require(!yaml_document.fold_regions().empty() &&
                yaml_document.fold_regions().front().start_line == 2,
            "indented YAML collections should expose fold regions");
    const auto invalid_yaml = zeus::format_yaml("name: [one, two\nenabled: true");
    require(!invalid_yaml.ok && invalid_yaml.issue.code == "INVALID_YAML",
            "invalid YAML should report a stable parse error");
    const auto alias_yaml = zeus::format_yaml("defaults: &base\n  enabled: true\ncopy: *base\n");
    require(!alias_yaml.ok && alias_yaml.issue.code == "YAML_UNSAFE_FEATURE",
            "YAML anchors and aliases should be rejected before parsing");
    const auto multi_yaml = zeus::format_yaml("---\na: 1\n---\nb: 2\n");
    require(!multi_yaml.ok && multi_yaml.issue.code == "YAML_MULTIPLE_DOCUMENTS",
            "multiple YAML documents should fail deterministically");
    require(zeus::process_text("hello: world").detected == zeus::ContentKind::Text,
            "a single colon-bearing line should remain plain text in auto mode");

    const auto converted_yaml = zeus::json_to_yaml(
        R"({"name":"Zeus","enabled":true,"stringBool":"true","count":2,"items":["json","xml"]})");
    require(converted_yaml.ok && converted_yaml.value.find("name: Zeus") != std::string::npos,
            "JSON should convert to readable block YAML");
    const auto json_yaml_roundtrip = zeus::yaml_to_json(converted_yaml.value);
    require(json_yaml_roundtrip.ok &&
                json_yaml_roundtrip.value.find("\"stringBool\": \"true\"") != std::string::npos,
            "JSON to YAML conversion should preserve strings that resemble YAML scalar types");
    const auto converted_json = zeus::yaml_to_json(
        "name: Zeus\nenabled: true\ncount: 2\nquoted: \"true\"\nitems:\n  - json\n  - xml\n");
    require(converted_json.ok && converted_json.value.find("\"enabled\": true") != std::string::npos &&
                converted_json.value.find("\"count\": 2") != std::string::npos &&
                converted_json.value.find("\"quoted\": \"true\"") != std::string::npos,
            "YAML to JSON should preserve core scalar types and quoted strings");
    require(zeus::format_json(converted_json.value).ok,
            "YAML conversion output should always be strict JSON");
    const auto converted_xml = zeus::json_to_xml(
        R"({"name":"Zeus & Tools","items":["json","xml"],"bad key":"<value>","none":null})");
    require(converted_xml.ok &&
                converted_xml.value.find("<name>Zeus &amp; Tools</name>") != std::string::npos &&
                converted_xml.value.find("<items>\n    <item>json</item>") != std::string::npos &&
                converted_xml.value.find("<entry key=\"bad key\">&lt;value&gt;</entry>") != std::string::npos &&
                converted_xml.value.find("<none />") != std::string::npos,
            "JSON to XML should map objects and arrays predictably and escape unsafe text");
    require(zeus::format_xml(converted_xml.value).ok,
            "JSON to XML conversion output should always be strict XML");
    const auto converted_xml_json = zeus::xml_to_json(
        R"(<catalog version="1"><item id="a">JSON</item><item id="b">XML</item></catalog>)");
    require(converted_xml_json.ok &&
                converted_xml_json.value.find("\"catalog\"") != std::string::npos &&
                converted_xml_json.value.find("\"@version\": \"1\"") != std::string::npos &&
                converted_xml_json.value.find("\"item\": [") != std::string::npos &&
                converted_xml_json.value.find("\"@id\": \"a\"") != std::string::npos &&
                converted_xml_json.value.find("\"#text\": \"JSON\"") != std::string::npos,
            "XML to JSON should preserve the root, attributes, text and repeated elements");
    require(zeus::format_json(converted_xml_json.value).ok,
            "XML to JSON conversion output should always be strict JSON");
    const auto mixed_xml_json = zeus::xml_to_json(
        R"(<p>Hello <strong>Zeus</strong>!</p>)");
    require(mixed_xml_json.ok &&
                mixed_xml_json.value.find("\"#content\": [") != std::string::npos &&
                mixed_xml_json.value.find("\"strong\": \"Zeus\"") != std::string::npos,
            "XML mixed content should retain document order in a content array");
    const auto unsafe_xml_json = zeus::xml_to_json(
        R"(<!DOCTYPE root [<!ENTITY x SYSTEM "file:///tmp/secret">]><root>&x;</root>)");
    require(!unsafe_xml_json.ok && unsafe_xml_json.issue.code == "XML_UNSAFE_DECLARATION",
            "XML to JSON should retain XML external entity protections");
    const auto converted_csv = zeus::json_to_csv(
        R"([{"name":"Zeus","age":2,"tags":["json","xml"]},{"name":"A, B","active":true}])");
    require(converted_csv.ok && converted_csv.value.find("name,age,tags,active") == 0 &&
                converted_csv.value.find("Zeus,2,\"[\"\"json\"\",\"\"xml\"\"]\",") != std::string::npos &&
                converted_csv.value.find("\"A, B\"") != std::string::npos,
            "JSON arrays of objects should convert to CSV with union headers and escaped cells");
    const auto reparsed_converted_csv = zeus::parse_csv(converted_csv.value);
    require(reparsed_converted_csv.ok && reparsed_converted_csv.document.rows.size() == 3 &&
                reparsed_converted_csv.document.rows[1][2] == R"(["json","xml"])",
            "JSON-derived CSV should round-trip through the CSV parser");
    const auto csv_case_insensitive = reparsed_converted_csv.document.search("zeus");
    require(csv_case_insensitive.size() == 1 && csv_case_insensitive.front().row == 1 &&
                csv_case_insensitive.front().start == 0 &&
                csv_case_insensitive.front().length == 4,
            "CSV search should retain cell coordinates and the exact matched byte range");
    require(reparsed_converted_csv.document.search("zeus", true).empty(),
            "CSV search should support case-sensitive matching");
    std::string csv_regex_error;
    const auto csv_regex = reparsed_converted_csv.document.search(
        "^(Zeus|A, B)$", true, true, &csv_regex_error);
    require(csv_regex_error.empty() && csv_regex.size() == 2,
            "CSV search should support regular expressions across cells");
    zeus::CsvDocument repeated_csv;
    repeated_csv.rows = {{"Zeus zeus ZEUS"}};
    const auto repeated_matches = repeated_csv.search("zeus");
    require(repeated_matches.size() == 3 && repeated_matches[0].start == 0 &&
                repeated_matches[1].start == 5 && repeated_matches[2].start == 10,
            "CSV search should return every exact occurrence inside one cell");
    reparsed_converted_csv.document.search("(", true, true, &csv_regex_error);
    require(!csv_regex_error.empty(), "CSV search should report invalid regular expressions");
    const auto processed_yaml_conversion = zeus::process_text(
        R"({"name":"Zeus"})", zeus::ProcessingMode::JsonToYaml);
    require(processed_yaml_conversion.ok && processed_yaml_conversion.detected == zeus::ContentKind::Yaml,
            "JSON to YAML processing should select YAML result highlighting");
    const auto processed_xml_conversion = zeus::process_text(
        R"({"name":"Zeus"})", zeus::ProcessingMode::JsonToXml);
    require(processed_xml_conversion.ok && processed_xml_conversion.detected == zeus::ContentKind::Xml,
            "JSON to XML processing should select XML result highlighting");
    const auto processed_xml_json_conversion = zeus::process_text(
        R"(<user id="1"><name>Zeus</name></user>)", zeus::ProcessingMode::XmlToJson);
    require(processed_xml_json_conversion.ok &&
                processed_xml_json_conversion.detected == zeus::ContentKind::Json &&
                processed_xml_json_conversion.structured,
            "XML to JSON processing should select JSON result highlighting");
    const auto processed_csv_conversion = zeus::process_text(
        R"([{"name":"Zeus"},{"name":"Tools"}])", zeus::ProcessingMode::JsonToCsv);
    require(processed_csv_conversion.ok && processed_csv_conversion.detected == zeus::ContentKind::Csv &&
                processed_csv_conversion.tabular,
            "JSON to CSV processing should select the table result view");
    const auto processed_json_conversion = zeus::process_text(
        "name: Zeus\nenabled: true\n", zeus::ProcessingMode::YamlToJson);
    require(processed_json_conversion.ok && processed_json_conversion.detected == zeus::ContentKind::Json,
            "YAML to JSON processing should select JSON result highlighting");

    const auto minified_json = zeus::process_text(
        "{\n  \"message\": \"hello world\",\n  \"value\": 2\n}",
        zeus::ProcessingMode::JsonMinify);
    require(minified_json.ok && minified_json.value ==
                R"({"message":"hello world","value":2})",
            "JSON minify should remove only insignificant whitespace");

    const auto escaped_json = zeus::process_text("{\"line\":\"a\nb\"}", zeus::ProcessingMode::JsonEscape);
    require(escaped_json.value == "{\\\"line\\\":\\\"a\\nb\\\"}",
            "JSON escape should escape quotes and control characters");

    require(zeus::process_text("Zeus 你好", zeus::ProcessingMode::Upper).value == "ZEUS 你好",
            "upper should transform ASCII and preserve UTF-8 bytes");
    require(zeus::process_text("ZeUs 你好", zeus::ProcessingMode::Lower).value == "zeus 你好",
            "lower should transform ASCII and preserve UTF-8 bytes");

    const auto decoded_base64 = zeus::process_text("eyJhbnN3ZXIiOjQyfQ==");
    require(decoded_base64.ok && decoded_base64.detected == zeus::ContentKind::Base64,
            "auto mode should detect high-confidence Base64");
    require(decoded_base64.decoded && decoded_base64.structured,
            "Base64 JSON should decode once and format as JSON");
    require(decoded_base64.value.find("\"answer\": 42") != std::string::npos,
            "decoded JSON should be formatted");

    const auto one_layer = zeus::process_text("WlhsS2FtSnNTV2xQYWtacVUxUkZPUT09");
    require(one_layer.decoded, "encoded content should decode");
    require(one_layer.value == "ZXlKamJsSWlPakZqU1RFOQ==",
            "automatic decoding must stop after exactly one layer");

    const auto decoded_url = zeus::process_text("message%3D%E4%BD%A0%E5%A5%BD");
    require(decoded_url.ok && decoded_url.detected == zeus::ContentKind::UrlEncoded,
            "auto mode should detect URL encoding");
    require(decoded_url.value == "message=你好", "URL decoding should preserve UTF-8");

    const auto jwt = zeus::process_text(
        "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
        "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiYWRtaW4iOnRydWV9."
        "TJVA95OrM7E2cBab30RMHrHDcEfxjoYZgeFONFh7HgQ");
    require(jwt.ok && jwt.detected == zeus::ContentKind::Jwt && jwt.structured,
            "auto mode should detect a valid three-part JWT");
    require(jwt.value.find("\"header\"") != std::string::npos &&
                jwt.value.find("\"name\": \"John Doe\"") != std::string::npos &&
                jwt.value.find("\"verification\": \"not verified\"") != std::string::npos,
            "JWT inspection should expose header and payload while marking the signature unverified");
    const auto jwt_document = zeus::HighlightedDocument::json(jwt.value);
    require(jwt_document.search("John Doe", true).size() == 1,
            "decoded JWT claims should remain searchable in the result view");
    const auto jwt_like_text = zeus::process_text("hello.world.token");
    require(jwt_like_text.detected == zeus::ContentKind::Text,
            "ordinary dotted text must not be misdetected as JWT");

    const auto invalid_url = zeus::process_text("bad%2", zeus::ProcessingMode::UrlDecode);
    require(!invalid_url.ok && invalid_url.error_code == "INVALID_URL_ENCODING",
            "manual URL decoding should report malformed percent sequences");

    const auto forced_invalid_json = zeus::process_text("{\n  \"x\": 01\n}", zeus::ProcessingMode::Json);
    require(!forced_invalid_json.ok && forced_invalid_json.error_line == 2,
            "manual JSON mode should preserve parser error coordinates");

    const auto short_base64 = zeus::process_text("dGVzdA==");
    require(short_base64.detected == zeus::ContentKind::Text,
            "short Base64-like strings should not be auto-decoded");
    const auto forced_base64 = zeus::process_text("dGVzdA==", zeus::ProcessingMode::Base64);
    require(forced_base64.ok && forced_base64.value == "test",
            "manual Base64 mode should decode valid short strings");

    const auto encoded_base64 = zeus::process_text("你好 Zeus", zeus::ProcessingMode::Base64Encode);
    require(encoded_base64.ok && encoded_base64.value == "5L2g5aW9IFpldXM=",
            "Base64 encode should support UTF-8 input and standard padding");

    const auto encoded_url = zeus::process_text("你好 Zeus/1", zeus::ProcessingMode::UrlEncode);
    require(encoded_url.ok && encoded_url.value == "%E4%BD%A0%E5%A5%BD%20Zeus%2F1",
            "URL encode should preserve unreserved characters and encode UTF-8 bytes");

    const auto binary_base64 = zeus::process_text("AAECAwQF", zeus::ProcessingMode::Base64);
    require(binary_base64.ok && binary_base64.label.find("Binary") != std::string::npos,
            "binary Base64 must render a safe summary instead of invalid text");

    const auto csv = zeus::parse_csv("name,description\nZeus,\"format, decode\"\n中文,工具");
    require(csv.ok && csv.document.rows.size() == 3,
            "CSV detector should parse a consistent multi-row table");
    require(csv.document.rows[1][1] == "format, decode",
            "CSV parser should preserve delimiters inside quoted fields");
    require(csv.document.to_tsv().find("Zeus\tformat, decode") != std::string::npos,
            "CSV should copy as TSV");

    const auto semicolon_csv = zeus::parse_csv("name;age\nAlice;30", ';', true);
    require(semicolon_csv.ok && semicolon_csv.document.delimiter == ';' &&
                semicolon_csv.document.rows[1][1] == "30",
            "CSV parser should honor a manually selected semicolon delimiter");
    const auto wrong_delimiter_csv = zeus::parse_csv("name;age\nAlice;30", ',', true);
    require(!wrong_delimiter_csv.ok,
            "CSV parser should reject a forced delimiter that does not produce a table");

    const auto detected_csv = zeus::process_text("name,age\nAlice,30\nBob,28");
    require(detected_csv.ok && detected_csv.detected == zeus::ContentKind::Csv && detected_csv.tabular,
            "auto mode should detect consistent CSV content");

    const auto multiline_csv = zeus::parse_csv("id,note\n1,\"line one\nline two\"");
    require(multiline_csv.ok && multiline_csv.document.rows[1][1].find('\n') != std::string::npos,
            "CSV parser should preserve newlines inside quoted fields");

    std::cout << "All Zeus core tests passed\n";
    return 0;
}
