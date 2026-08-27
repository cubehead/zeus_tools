#include "detection_corpus.h"

#include "zeus/json_formatter.h"
#include "zeus/secure_memory.h"
#include "zeus/crypto_service.h"
#include "zeus/csv_document.h"
#include "zeus/developer_tools.h"
#include "zeus/ini_formatter.h"
#include "zeus/mongo_shell_formatter.h"
#include "zeus/text_document.h"
#include "zeus/text_selection.h"
#include "zeus/text_processor.h"
#include "zeus/structured_converter.h"
#include "zeus/toml_formatter.h"
#include "zeus/xml_formatter.h"
#include "zeus/yaml_formatter.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void verify_detection_corpus() {
    const std::string path = std::string(ZEUS_TEST_FIXTURE_DIR) + "/detection-v1.tsv";
    const test_support::DetectionCorpus corpus =
        test_support::load_detection_corpus(path);
    if (!corpus.error.empty()) {
        std::cerr << "FAILED: " << corpus.error << '\n';
        std::exit(1);
    }

    for (const auto& item : corpus.cases) {
        const auto result = zeus::process_text(item.input);
        if (result.ok != item.expected_ok ||
            std::string(zeus::content_kind_name(result.detected)) != item.expected_kind) {
            std::cerr << "FAILED: detection corpus case '" << item.name << "' expected "
                      << item.expected_kind << " (ok=" << item.expected_ok << ") but got "
                      << zeus::content_kind_name(result.detected) << " (ok="
                      << result.ok << ")\n";
            std::exit(1);
        }
    }
    require(corpus.cases.size() >= 50,
            "detection corpus should retain broad boundary coverage");
}

} // namespace

int main() {
    verify_detection_corpus();
    std::string sensitive = "temporary secret";
    zeus::secure_zero(sensitive.data(), sensitive.size());
    require(std::all_of(sensitive.begin(), sensitive.end(), [](char value) {
                return value == '\0';
            }),
            "secure zeroing should overwrite every requested byte");
    sensitive = "replacement secret";
    zeus::secure_clear(sensitive);
    require(sensitive.empty(), "secure string clearing should reset its logical value");
    const auto toml = zeus::format_toml(
        "title=\"Zeus Tools\"\n[window]\nwidth=1200\ndark=true");
    require(toml.ok && toml.value.find("[window]") != std::string::npos,
            "valid TOML should format");
    const auto toml_json = zeus::toml_to_json(toml.value);
    require(toml_json.ok && toml_json.value.find("\"width\": 1200") != std::string::npos,
            "TOML should convert to typed JSON");
    const auto json_toml = zeus::json_to_toml(
        R"({"title":"Zeus","window":{"width":1200,"dark":true},"ports":[80,443]})");
    require(json_toml.ok && json_toml.value.find("[window]") != std::string::npos &&
                json_toml.value.find("ports = [ 80, 443 ]") != std::string::npos,
            "JSON objects should convert to TOML tables and arrays");
    require(!zeus::json_to_toml(R"({"missing":null})").ok,
            "JSON null should report its unsupported TOML mapping");
    std::string malformed_toml_header(256, '[');
    malformed_toml_header.append(256, ']');
    const auto malformed_toml = zeus::format_toml(malformed_toml_header);
    require(!malformed_toml.ok && malformed_toml.issue.code == "PARSE_TOML" &&
                malformed_toml.issue.line == 1,
            "malformed TOML table headers should fail before reaching parser assertions");
    const auto multiline_toml = zeus::format_toml(
        "value = \"\"\"\n[[[ remains string content\n\"\"\"");
    require(multiline_toml.ok,
            "TOML table preflight should ignore bracket-like multiline string content");

    const auto ini = zeus::format_ini("[window]\nwidth=1200\ntheme: system");
    require(ini.ok && ini.value.find("width = 1200") != std::string::npos,
            "INI and Properties assignments should normalize conservatively");
    const auto ini_json = zeus::ini_to_json(ini.value);
    require(ini_json.ok && ini_json.value.find("\"width\": \"1200\"") != std::string::npos,
            "INI conversion should preserve values as JSON strings");
    require(!zeus::format_ini("name=one\nname=two").ok,
            "duplicate INI keys should fail instead of losing data");

    require(zeus::encode_html_entities("<Zeus & Tools>") ==
                "&lt;Zeus &amp; Tools&gt;",
            "HTML entity encoding should escape markup characters");
    const auto html = zeus::decode_html_entities("Zeus &amp; &#x4F60;&#22909;");
    require(html.ok && html.value == "Zeus & 你好",
            "HTML entity decoding should support named and numeric Unicode entities");
    require(!zeus::decode_html_entities("plain text").ok,
            "HTML entity decoding should reject inputs with no entity");

    require(zeus::encode_hex("Zeus") == "5a657573",
            "Hex encoding should use UTF-8 bytes");
    const auto hex = zeus::decode_hex("0x5a 65:75-73");
    require(hex.ok && hex.value == "Zeus",
            "Hex decoding should accept explicit prefixes and byte separators");
    require(!zeus::looks_like_hex_encoding("deadbeefcafebabe"),
            "plain hash-like text should not be automatically decoded as Hex");

    const auto timestamp = zeus::format_unix_timestamp("1700000000");
    require(timestamp.ok && timestamp.value.find("UTC: 2023-11-14T22:13:20.000Z") != std::string::npos,
            "Unix seconds should convert to a stable UTC timestamp");
    require(zeus::looks_like_unix_timestamp("1700000000000") &&
                !zeus::looks_like_unix_timestamp("order-1700000000"),
            "timestamp suggestions should require exact 10 or 13 digit input");
    const std::string embedded_timestamp = "x1700000000y";
    require(zeus::looks_like_unix_timestamp(
                std::string_view(embedded_timestamp).substr(1, 10)),
            "timestamp suggestions should parse bounded non-owning input");
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
    require(zeus::compute_digest("abc", zeus::DigestAlgorithm::Sha384).hex ==
                "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded163"
                "1a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7",
            "SHA-384 should match its standard abc vector");
    require(zeus::compute_digest("abc", zeus::DigestAlgorithm::Sha512).hex ==
                "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f",
            "SHA-512 should match its standard abc vector");
    const auto crc32 = zeus::compute_digest("abc", zeus::DigestAlgorithm::Crc32);
    require(crc32.ok && crc32.hex == "352441c2" && crc32.base64 == "NSRBwg==",
            "CRC32 should match the standard IEEE abc vector");
    require(!zeus::compute_hmac("abc", "key", zeus::DigestAlgorithm::Crc32).ok,
            "CRC32 should be rejected as an HMAC algorithm");

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
    require(zeus::compute_hmac(hmac_message, "key", zeus::DigestAlgorithm::Sha384).hex ==
                "d7f4727e2c0b39ae0f1e40cc96f60242d5b7801841cea6fc592c5d3e1ae50700"
                "582a96cf35e1e554995fe4e03381c237",
            "HMAC-SHA384 should match the standard vector");
    require(zeus::compute_hmac(hmac_message, "key", zeus::DigestAlgorithm::Sha512).hex ==
                "b42af09057bac1e2d41708e48a902e09b5ff7f12ab428a4fe86653c73dd248fb"
                "82f948a549f7b791a5b41915ee4d1ec3935357e4e2317250d0372afa2ebeeb3a",
            "HMAC-SHA512 should match the standard vector");
    require(zeus::compute_hmac_encoded(
                hmac_message, "6b6579", zeus::HmacKeyEncoding::Hex,
                zeus::DigestAlgorithm::Sha256).hex ==
                "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8",
            "hex-encoded HMAC keys should decode to raw key bytes");
    require(zeus::compute_hmac_encoded(
                hmac_message, "a2V5", zeus::HmacKeyEncoding::Base64,
                zeus::DigestAlgorithm::Sha256).hex ==
                "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8",
            "Base64-encoded HMAC keys should decode to raw key bytes");
    require(!zeus::compute_hmac_encoded(
                hmac_message, "abc", zeus::HmacKeyEncoding::Hex,
                zeus::DigestAlgorithm::Sha256).ok,
            "invalid encoded HMAC keys should return an error");
    require(!zeus::compute_hmac_encoded(
                hmac_message, "a2V5=", zeus::HmacKeyEncoding::Base64,
                zeus::DigestAlgorithm::Sha256).ok,
            "non-canonical Base64 key padding should return an error");
    require(zeus::compute_hmac_encoded(
                hmac_message, "+/8=", zeus::HmacKeyEncoding::Base64,
                zeus::DigestAlgorithm::Sha256).ok &&
                zeus::compute_hmac_encoded(
                    hmac_message, "-_8=", zeus::HmacKeyEncoding::Base64,
                    zeus::DigestAlgorithm::Sha256).ok,
            "standard and URL-safe Base64 HMAC keys should both be accepted");
    require(!zeus::compute_hmac_encoded(
                hmac_message, "+_8=", zeus::HmacKeyEncoding::Base64,
                zeus::DigestAlgorithm::Sha256).ok,
            "mixed Base64 alphabets must be rejected for HMAC keys");
    require(zeus::digest_algorithm_is_weak(zeus::DigestAlgorithm::Md5) &&
                zeus::digest_algorithm_is_weak(zeus::DigestAlgorithm::Sha1) &&
                !zeus::digest_algorithm_is_weak(zeus::DigestAlgorithm::Sha256) &&
                zeus::digest_algorithm_is_checksum(zeus::DigestAlgorithm::Crc32),
            "MD5 and SHA-1 should be marked as weak algorithms");

    const auto decode_layer_base64 = zeus::process_text(
        "YUdWc2JHOD0=", zeus::ProcessingMode::DecodeOneLayer);
    require(decode_layer_base64.ok && decode_layer_base64.decoded &&
                decode_layer_base64.value == "aGVsbG8=",
            "manual continuation should decode exactly one Base64 layer");
    const auto decode_layer_url = zeus::process_text(
        "%2561%2562%2563", zeus::ProcessingMode::DecodeOneLayer);
    require(decode_layer_url.ok && decode_layer_url.value == "%61%62%63",
            "manual continuation should decode exactly one URL layer");
    const auto decode_layer_unicode = zeus::process_text(
        R"(\\u4F60\\u597D)", zeus::ProcessingMode::DecodeOneLayer);
    require(decode_layer_unicode.ok && decode_layer_unicode.decoded &&
                decode_layer_unicode.value == R"(\u4F60\u597D)",
            "manual continuation should remove exactly one Unicode escape layer");
    const auto no_decode_layer = zeus::process_text(
        "plain text", zeus::ProcessingMode::DecodeOneLayer);
    require(!no_decode_layer.ok && no_decode_layer.error_code == "NO_ENCODED_LAYER",
            "manual continuation should reject plain text");

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
    const std::size_t expanded_revision = folds.revision();
    require(folds.set_collapsed(document, 0, true) && folds.is_collapsed(0),
            "an expanded JSON container should support explicit collapse");
    require(!folds.set_collapsed(document, 0, true) &&
                folds.revision() == expanded_revision + 1,
            "repeating explicit collapse should be idempotent");
    require(folds.set_collapsed(document, 0, false) && !folds.is_collapsed(0),
            "a collapsed JSON container should support explicit expansion");
    require(!folds.set_collapsed(document, document.lines().size(), true),
            "explicit collapse should reject lines without a fold region");

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
    require(detected_json.output_kind == zeus::ContentKind::Json,
            "detected JSON should expose JSON as its output kind");

    const std::string mongo_shell =
        R"([{"field":"detailsDTOs[0].id","before":new NumberInt("421864"),)"
        R"("long":NumberLong("9223372036854775807"),)"
        R"("price":NumberDecimal("12.50"),)"
        R"("id":ObjectId("507f1f77bcf86cd799439011"),)"
        R"("createdAt":ISODate("2026-08-21T10:15:30Z")}])";
    const auto converted_mongo = zeus::convert_mongo_shell_to_extended_json(mongo_shell);
    require(converted_mongo.json.ok && converted_mongo.converted_constructors == 5,
            "MongoDB Shell constructors should convert without executing JavaScript");
    require(converted_mongo.json.value.find(R"("$numberInt": "421864")") !=
                std::string::npos &&
            converted_mongo.json.value.find(R"("$oid": "507f1f77bcf86cd799439011")") !=
                std::string::npos,
            "MongoDB Shell conversion should preserve BSON types as Extended JSON");
    const auto detected_mongo = zeus::process_text(mongo_shell);
    require(detected_mongo.ok && detected_mongo.detected == zeus::ContentKind::MongoShell &&
                detected_mongo.output_kind == zeus::ContentKind::Json,
            "automatic detection should recognize a complete MongoDB Shell document");
    const auto explicit_mongo = zeus::process_text(
        R"({"value":new NumberInt("7")})", zeus::ProcessingMode::MongoShell);
    require(explicit_mongo.ok && explicit_mongo.value.find("$numberInt") != std::string::npos,
            "manual MongoDB input should convert to Extended JSON");
    const auto common_mongo_literals = zeus::process_text(
        R"({"count":NumberInt(421864),"debt":new NumberLong(-9),)"
        R"("id":ObjectId('507f1f77bcf86cd799439011'),)"
        R"("createdAt":ISODate('2026-08-21T10:15:30Z')})",
        zeus::ProcessingMode::MongoShell);
    require(common_mongo_literals.ok &&
                common_mongo_literals.value.find(R"("$numberInt": "421864")") !=
                    std::string::npos &&
                common_mongo_literals.value.find(R"("$numberLong": "-9")") !=
                    std::string::npos,
            "MongoDB integer literals and single-quoted constructor values should be accepted");
    const auto mongosh_literals = zeus::process_text(
        R"({"count":Int32(7),"long":Long(9223372036854775807),"ratio":Double(1.25e-3),)"
        R"("special":Double('-Infinity'),)"
        R"("traceId":UUID('3b241101-e2bb-4255-8caf-4136c566a962')})",
        zeus::ProcessingMode::MongoShell);
    require(mongosh_literals.ok &&
                mongosh_literals.value.find(R"("$numberInt": "7")") !=
                    std::string::npos &&
                mongosh_literals.value.find(
                    R"("$numberLong": "9223372036854775807")") !=
                    std::string::npos &&
                mongosh_literals.value.find(R"("$numberDouble": "1.25e-3")") !=
                    std::string::npos &&
                mongosh_literals.value.find(
                    R"("$uuid": "3b241101-e2bb-4255-8caf-4136c566a962")") !=
                    std::string::npos,
            "mongosh Int32, Long, Double and UUID values should preserve their BSON types");
    const auto timestamps = zeus::process_text(
        R"({"current":Timestamp({"t":1724212800,"i":3}),)"
        R"("legacy":Timestamp(1724212801,4)})",
        zeus::ProcessingMode::MongoShell);
    require(timestamps.ok &&
                timestamps.value.find(
                    R"("$timestamp": {)"
                    "\n      \"t\": 1724212800,\n      \"i\": 3") !=
                    std::string::npos &&
                timestamps.value.find("\"t\": 1724212801") != std::string::npos,
            "current and legacy Timestamp signatures should convert to canonical Extended JSON");
    const auto binary_and_bounds = zeus::process_text(
        R"({"payload":BinData(4,'AQIDBA=='),"lowest":MinKey(),"highest":new MaxKey()})",
        zeus::ProcessingMode::MongoShell);
    require(binary_and_bounds.ok &&
                binary_and_bounds.value.find(R"("base64": "AQIDBA==")") !=
                    std::string::npos &&
                binary_and_bounds.value.find(R"("subType": "04")") !=
                    std::string::npos &&
                binary_and_bounds.value.find(R"("$minKey": 1)") !=
                    std::string::npos &&
                binary_and_bounds.value.find(R"("$maxKey": 1)") !=
                    std::string::npos,
            "BinData, MinKey and MaxKey should convert to canonical Extended JSON");
    const auto hex_binary = zeus::process_text(
        R"({payload:HexData(4,'123456abcdef'),empty:HexData(0,'')})",
        zeus::ProcessingMode::MongoShell);
    require(hex_binary.ok &&
                hex_binary.value.find(R"("base64": "EjRWq83v")") !=
                    std::string::npos &&
                hex_binary.value.find(R"("subType": "04")") !=
                    std::string::npos &&
                hex_binary.value.find(R"("base64": "")") !=
                    std::string::npos,
            "HexData should strictly decode complete bytes and reuse standard Base64 output");
    const auto displayed_binary = zeus::process_text(
        R"({generic:Binary.createFromBase64('SGVsbG8='),uuid:Binary.createFromBase64('AQIDBA==',4),hex:Binary.createFromHexString('123456abcdef')})",
        zeus::ProcessingMode::MongoShell);
    require(displayed_binary.ok &&
                displayed_binary.value.find(R"("base64": "SGVsbG8=")") !=
                    std::string::npos &&
                displayed_binary.value.find(R"("subType": "00")") !=
                    std::string::npos &&
                displayed_binary.value.find(R"("subType": "04")") !=
                    std::string::npos &&
                displayed_binary.value.find(R"("base64": "EjRWq83v")") !=
                    std::string::npos,
            "mongosh Binary display constructors should convert to canonical Extended JSON");
    const auto object_id_factories = zeus::process_text(
        R"({base64:ObjectId.createFromBase64('SGVsbG8gV29ybGQh'),hex:ObjectId.createFromHexString('64C13AB08EDF48A008793CAC'),direct:ObjectId('507F1F77BCF86CD799439011')})",
        zeus::ProcessingMode::MongoShell);
    require(object_id_factories.ok &&
                object_id_factories.value.find(
                    R"("$oid": "48656c6c6f20576f726c6421")") != std::string::npos &&
                object_id_factories.value.find(
                    R"("$oid": "64c13ab08edf48a008793cac")") != std::string::npos &&
                object_id_factories.value.find(
                    R"("$oid": "507f1f77bcf86cd799439011")") != std::string::npos,
            "ObjectId factories and direct values should emit canonical lowercase hex");
    const auto code_and_canonical_integers = zeus::process_text(
        R"({script:new Code('function () { return 1; }'),padded:NumberInt('001'),zero:NumberLong('-0')})",
        zeus::ProcessingMode::MongoShell);
    require(code_and_canonical_integers.ok &&
                code_and_canonical_integers.value.find(
                    R"("$code": "function () { return 1; }")") != std::string::npos &&
                code_and_canonical_integers.value.find(R"("$numberInt": "1")") !=
                    std::string::npos &&
                code_and_canonical_integers.value.find(R"("$numberLong": "0")") !=
                    std::string::npos,
            "Code should remain inert text and integer wrappers should emit canonical values");
    const auto dates = zeus::process_text(
        R"({"released":new Date('2026-08-21T10:15:30Z'),"epoch":new Date(0)})",
        zeus::ProcessingMode::MongoShell);
    require(dates.ok &&
                dates.value.find(R"("$date": "2026-08-21T10:15:30Z")") !=
                    std::string::npos &&
                dates.value.find(R"("$numberLong": "0")") !=
                    std::string::npos,
            "ISO and millisecond Date values should use their correct Extended JSON forms");
    const auto shell_object_syntax = zeus::process_text(
        R"({count:Int32(7),name:'Zeus Tools',nested:{active:true,id:ObjectId('507f1f77bcf86cd799439011')}})",
        zeus::ProcessingMode::MongoShell);
    require(shell_object_syntax.ok &&
                shell_object_syntax.value.find(R"("count": {)") != std::string::npos &&
                shell_object_syntax.value.find(R"("name": "Zeus Tools")") !=
                    std::string::npos &&
                shell_object_syntax.value.find(R"("nested": {)") !=
                    std::string::npos,
            "unquoted keys and single-quoted shell strings should normalize to strict JSON");
    const auto regex_literals = zeus::process_text(
        R"({name:/^zeus\/tools$/mi,digits:/\d+/,items:[/^one/i,/two$/]})");
    require(regex_literals.ok &&
                regex_literals.detected == zeus::ContentKind::MongoShell &&
                regex_literals.value.find(R"("pattern": "^zeus/tools$")") !=
                    std::string::npos &&
                regex_literals.value.find(R"("options": "im")") !=
                    std::string::npos &&
                regex_literals.value.find(R"("pattern": "\\d+")") !=
                    std::string::npos,
            "MongoDB regex literals should preserve patterns and sort supported options");
    const auto bson_regex = zeus::process_text(
        R"({name:BSONRegExp('^zeus\\d+$','mi'),empty:BSONRegExp('plain','')})",
        zeus::ProcessingMode::MongoShell);
    require(bson_regex.ok &&
                bson_regex.value.find(R"("pattern": "^zeus\\d+$")") !=
                    std::string::npos &&
                bson_regex.value.find(R"("options": "im")") !=
                    std::string::npos &&
                bson_regex.value.find(R"("options": "")") !=
                    std::string::npos,
            "BSONRegExp should preserve patterns and canonicalize supported flags");
    require(!zeus::process_text(
                R"({"value":new NumberInt("2147483648")})",
                zeus::ProcessingMode::MongoShell).ok,
            "NumberInt values outside the signed 32-bit range should be rejected");
    require(!zeus::process_text(
                R"({"value":Long("9223372036854775808")})",
                zeus::ProcessingMode::MongoShell).ok,
            "Long values outside the signed 64-bit range should be rejected");
    require(!zeus::process_text(
                R"({"value":new Evil("1")})",
                zeus::ProcessingMode::MongoShell).ok,
            "unknown MongoDB Shell constructors should be rejected");
    require(!zeus::process_text(
                R"({"value":NumberInt(1 + 2)})",
                zeus::ProcessingMode::MongoShell).ok,
            "MongoDB constructor arguments must not evaluate expressions");
    require(!zeus::process_text(
                R"({"value":UUID('not-a-uuid')})",
                zeus::ProcessingMode::MongoShell).ok,
            "invalid UUID constructor values should be rejected");
    require(!zeus::process_text(
                R"({"value":Timestamp({"t":4294967296,"i":1})})",
                zeus::ProcessingMode::MongoShell).ok,
            "Timestamp components outside the unsigned 32-bit range should be rejected");
    require(!zeus::process_text(
                R"({"value":BinData(256,"AQIDBA==")})",
                zeus::ProcessingMode::MongoShell).ok &&
                !zeus::process_text(
                    R"({"value":BinData(4,"AQIDB-_=")})",
                    zeus::ProcessingMode::MongoShell).ok,
            "BinData should reject invalid subtypes and non-canonical Base64");
    require(!zeus::process_text(
                R"({"value":HexData(0,"123xz")})",
                zeus::ProcessingMode::MongoShell).ok &&
                !zeus::process_text(
                    R"({"value":HexData(256,"12")})",
                    zeus::ProcessingMode::MongoShell).ok,
            "HexData should reject partial, odd-length and out-of-range input");
    require(!zeus::process_text(
                R"({"value":Binary.createFromBase64("AQIDB-_=")})",
                zeus::ProcessingMode::MongoShell).ok &&
                !zeus::process_text(
                    R"({"value":Binary.createFromBase64("AQIDBA==",256)})",
                    zeus::ProcessingMode::MongoShell).ok &&
                !zeus::process_text(
                    R"({"value":Binary.createFromHexString("123")})",
                    zeus::ProcessingMode::MongoShell).ok,
            "Binary display constructors should reject malformed payloads and subtypes");
    require(!zeus::process_text(
                R"({"value":ObjectId.createFromBase64("SGVsbG8gV29ybGQ=")})",
                zeus::ProcessingMode::MongoShell).ok &&
                !zeus::process_text(
                    R"({"value":ObjectId.createFromHexString("64c13ab08edf48a008793ca")})",
                    zeus::ProcessingMode::MongoShell).ok,
            "ObjectId factories should require exactly 12 decoded bytes or 24 hex characters");
    require(!zeus::process_text(
                R"({"value":new Date()})",
                zeus::ProcessingMode::MongoShell).ok,
            "nondeterministic empty Date constructors should be rejected");
    require(!zeus::process_text(
                R"({"value":new Code("return secret", {secret: 1})})",
                zeus::ProcessingMode::MongoShell).ok,
            "Code scope objects should remain unsupported until nested arguments are validated");
    require(!zeus::process_text(
                R"({value:someVariable,id:ObjectId('507f1f77bcf86cd799439011')})",
                zeus::ProcessingMode::MongoShell).ok,
            "bare variables in MongoDB Shell objects should remain forbidden");
    const auto unsupported_regex = zeus::process_text(R"({name:/zeus/g})");
    require(!unsupported_regex.ok &&
                unsupported_regex.detected == zeus::ContentKind::MongoShell &&
                unsupported_regex.error_code == "MONGO_SHELL_REGEX_OPTION",
            "unsupported global MongoDB regex options should report a concise error");
    require(!zeus::process_text(
                R"({name:BSONRegExp('zeus','g')})",
                zeus::ProcessingMode::MongoShell).ok &&
                !zeus::process_text(
                    R"({name:BSONRegExp('zeus','ii')})",
                    zeus::ProcessingMode::MongoShell).ok,
            "BSONRegExp should reject unsupported or repeated flags");
    require(zeus::process_text(R"({"value":1/2})").detected !=
                zeus::ContentKind::MongoShell,
            "division-like invalid JSON should not be mistaken for a MongoDB regex literal");
    const auto malformed_auto_mongo = zeus::process_text(
        R"({"value":new NumberInt("2147483648")})");
    require(!malformed_auto_mongo.ok &&
                malformed_auto_mongo.detected == zeus::ContentKind::MongoShell &&
                malformed_auto_mongo.error_code == "MONGO_SHELL_VALUE",
            "structured MongoDB-like input should retain concise conversion errors in Auto mode");
    require(zeus::process_text(R"(NumberInt("7") is documentation text)").detected ==
                zeus::ContentKind::Text,
            "constructor-like prose should not be accepted without complete JSON structure");

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
    require(detected_xml.ok && detected_xml.detected == zeus::ContentKind::Xml &&
                detected_xml.output_kind == zeus::ContentKind::Xml,
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
    require(detected_yaml.ok && detected_yaml.detected == zeus::ContentKind::Yaml &&
                detected_yaml.output_kind == zeus::ContentKind::Yaml,
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
    require(processed_yaml_conversion.ok &&
                processed_yaml_conversion.detected == zeus::ContentKind::Json &&
                processed_yaml_conversion.output_kind == zeus::ContentKind::Yaml,
            "JSON to YAML processing should retain its source and select YAML output");
    const auto processed_xml_conversion = zeus::process_text(
        R"({"name":"Zeus"})", zeus::ProcessingMode::JsonToXml);
    require(processed_xml_conversion.ok &&
                processed_xml_conversion.detected == zeus::ContentKind::Json &&
                processed_xml_conversion.output_kind == zeus::ContentKind::Xml,
            "JSON to XML processing should retain its source and select XML output");
    const auto processed_xml_json_conversion = zeus::process_text(
        R"(<user id="1"><name>Zeus</name></user>)", zeus::ProcessingMode::XmlToJson);
    require(processed_xml_json_conversion.ok &&
                processed_xml_json_conversion.detected == zeus::ContentKind::Xml &&
                processed_xml_json_conversion.output_kind == zeus::ContentKind::Json,
            "XML to JSON processing should retain its source and select JSON output");
    const auto processed_csv_conversion = zeus::process_text(
        R"([{"name":"Zeus"},{"name":"Tools"}])", zeus::ProcessingMode::JsonToCsv);
    require(processed_csv_conversion.ok &&
                processed_csv_conversion.detected == zeus::ContentKind::Json &&
                processed_csv_conversion.output_kind == zeus::ContentKind::Csv,
            "JSON to CSV processing should retain its source and select the table output");
    const auto processed_json_conversion = zeus::process_text(
        "name: Zeus\nenabled: true\n", zeus::ProcessingMode::YamlToJson);
    require(processed_json_conversion.ok &&
                processed_json_conversion.detected == zeus::ContentKind::Yaml &&
                processed_json_conversion.output_kind == zeus::ContentKind::Json,
            "YAML to JSON processing should retain its source and select JSON output");

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
    require(decoded_base64.output_kind == zeus::ContentKind::Json,
            "Base64 JSON should retain Base64 as its source and JSON as its output");
    require(decoded_base64.decoded && decoded_base64.output_kind == zeus::ContentKind::Json,
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
    require(decoded_url.output_kind == zeus::ContentKind::Text,
            "decoded URL text should expose Text as its output kind");
    require(decoded_url.value == "message=你好", "URL decoding should preserve UTF-8");

    const auto jwt = zeus::process_text(
        "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
        "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiYWRtaW4iOnRydWV9."
        "TJVA95OrM7E2cBab30RMHrHDcEfxjoYZgeFONFh7HgQ");
    require(jwt.ok && jwt.detected == zeus::ContentKind::Jwt &&
                jwt.output_kind == zeus::ContentKind::Json,
            "auto mode should detect a valid three-part JWT");
    require(jwt.output_kind == zeus::ContentKind::Json,
            "JWT inspection should expose JSON as its presentation output");
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
    require(encoded_base64.output_kind == zeus::ContentKind::Base64,
            "Base64 encode should expose Base64 as its output kind");

    const auto encoded_url = zeus::process_text("你好 Zeus/1", zeus::ProcessingMode::UrlEncode);
    require(encoded_url.ok && encoded_url.value == "%E4%BD%A0%E5%A5%BD%20Zeus%2F1",
            "URL encode should preserve unreserved characters and encode UTF-8 bytes");
    require(encoded_url.output_kind == zeus::ContentKind::UrlEncoded,
            "URL encode should expose URL encoding as its output kind");

    const auto decoded_unicode = zeus::process_text(
        R"(Hello \u4F60\u597D \uD83E\uDDEA)");
    require(decoded_unicode.ok && decoded_unicode.decoded &&
                decoded_unicode.detected == zeus::ContentKind::UnicodeEscaped &&
                decoded_unicode.value == "Hello 你好 🧪",
            "automatic detection should decode valid Unicode escapes once");
    const auto encoded_unicode = zeus::process_text(
        "你好 🧪\n", zeus::ProcessingMode::UnicodeEncode);
    require(encoded_unicode.ok &&
                encoded_unicode.output_kind == zeus::ContentKind::UnicodeEscaped &&
                encoded_unicode.value == R"(\u4F60\u597D \uD83E\uDDEA\u000A)",
            "Unicode escape should encode BMP, supplementary and control code points");
    const auto unicode_round_trip = zeus::process_text(
        encoded_unicode.value, zeus::ProcessingMode::UnicodeDecode);
    require(unicode_round_trip.ok && unicode_round_trip.value == "你好 🧪\n",
            "Unicode escape and unescape should round-trip UTF-8 text");
    const auto invalid_unicode = zeus::process_text(
        R"(\uD800)", zeus::ProcessingMode::UnicodeDecode);
    require(!invalid_unicode.ok && invalid_unicode.error_code == "INVALID_UNICODE_ESCAPE",
            "Unicode unescape should reject an unpaired surrogate");
    require(zeus::process_text(R"(C:\users\zeus)").detected == zeus::ContentKind::Text,
            "ordinary backslash text must not be misdetected as Unicode escapes");
    require(zeus::process_text(R"(<root>\u4F60</root>)").detected ==
                zeus::ContentKind::Xml,
            "structured formats should take precedence over nested Unicode escape text");

    const auto binary_base64 = zeus::process_text("AAECAwQF", zeus::ProcessingMode::Base64);
    require(binary_base64.ok && binary_base64.label.find("Binary") != std::string::npos,
            "binary Base64 must render a safe summary instead of invalid text");
    require(binary_base64.binary_data && binary_base64.binary_data->size() == 6 &&
                static_cast<unsigned char>((*binary_base64.binary_data)[0]) == 0 &&
                static_cast<unsigned char>((*binary_base64.binary_data)[5]) == 5,
            "binary Base64 must retain the exact decoded bytes for explicit export");
    require(binary_base64.binary_extension == ".bin",
            "unknown binary Base64 should use the safe generic extension");
    require(zeus::process_text("+/8=", zeus::ProcessingMode::Base64).ok &&
                zeus::process_text("-_8=", zeus::ProcessingMode::Base64).ok,
            "standard and URL-safe Base64 payloads should both decode");
    require(!zeus::process_text("+_8=", zeus::ProcessingMode::Base64).ok,
            "a payload mixing standard and URL-safe Base64 alphabets must be rejected");
    const auto data_url_text = zeus::process_text(
        "data:text/plain;charset=utf-8;base64,SGVsbG8sIFpldXMh");
    require(data_url_text.ok && data_url_text.decoded &&
                data_url_text.detected == zeus::ContentKind::Base64 &&
                data_url_text.value == "Hello, Zeus!" &&
                data_url_text.label.find("Data URL") != std::string::npos,
            "a Base64 Data URL should auto-decode its payload once");
    const auto uppercase_data_url = zeus::process_text(
        "DATA:text/plain;BASE64,SGVsbG8=", zeus::ProcessingMode::Base64);
    require(uppercase_data_url.ok && uppercase_data_url.value == "Hello",
            "Base64 Data URL scheme and marker matching should be ASCII case-insensitive");
    const auto data_url_png = zeus::process_text(
        "data:image/png;base64,"
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAusB9Y9Zr6sAAAAASUVORK5CYII=");
    require(data_url_png.ok && data_url_png.binary_data &&
                data_url_png.binary_extension == ".png",
            "a self-described binary Base64 Data URL should auto-decode for safe export");
    require(data_url_png.image_preview_source.rfind("data:image/png;base64,", 0) == 0,
            "verified PNG bytes from a Base64 Data URL should expose an in-memory preview");
    require(data_url_png.value.find("Dimensions: 1 × 1 px") != std::string::npos,
            "a valid PNG preview should include its pixel dimensions in the summary");
    const auto truncated_png = zeus::process_text(
        "data:image/png;base64,iVBORw0KGgo=", zeus::ProcessingMode::Base64);
    require(truncated_png.binary_extension == ".png" &&
                truncated_png.image_preview_source.empty(),
            "a truncated PNG signature should remain exportable but must not be previewed");
    const auto ordinary_png = zeus::process_text(
        data_url_png.image_preview_source.substr(std::string("data:image/png;base64,").size()),
        zeus::ProcessingMode::Base64);
    require(ordinary_png.image_preview_source.empty(),
            "ordinary Base64 image bytes should not opt into automatic image preview");
    const auto fake_png = zeus::process_text(
        "data:image/png;base64,JVBERi0xLjcK", zeus::ProcessingMode::Base64);
    require(fake_png.binary_extension == ".pdf" && fake_png.image_preview_source.empty(),
            "Data URL MIME metadata must not override magic-byte image detection");
    const std::string jpeg_header(
        "\xFF\xD8\xFF\xC0\x00\x11\x08\x00\x02\x00\x03\x03\x01\x11\x00\x02\x11\x00\x03\x11\x00\xFF\xD9",
        23);
    const auto jpeg_encoded = zeus::process_text(jpeg_header, zeus::ProcessingMode::Base64Encode);
    const auto data_url_jpeg = zeus::process_text(
        "data:image/jpeg;base64," + jpeg_encoded.value, zeus::ProcessingMode::Base64);
    require(data_url_jpeg.binary_extension == ".jpg" &&
                data_url_jpeg.image_preview_source.rfind("data:image/jpeg;base64,", 0) == 0,
            "verified JPEG bytes from a Base64 Data URL should expose an in-memory preview");
    require(data_url_jpeg.value.find("Dimensions: 3 × 2 px") != std::string::npos,
            "a JPEG SOF marker should provide width and height without decoding pixels");
    std::string oversized_png("\x89PNG\r\n\x1A\n\x00\x00\x00\x0DIHDR", 16);
    oversized_png += std::string("\x00\x00\x23\x29\x00\x00\x00\x01", 8);
    const auto oversized_encoded = zeus::process_text(
        oversized_png, zeus::ProcessingMode::Base64Encode);
    const auto oversized_data_url = zeus::process_text(
        "data:image/png;base64," + oversized_encoded.value, zeus::ProcessingMode::Base64);
    require(oversized_data_url.binary_extension == ".png" &&
                oversized_data_url.image_preview_source.empty(),
            "an image above the preview dimension limit should remain exportable without rendering");
    require(zeus::process_text("data:text/plain,Hello").detected ==
                zeus::ContentKind::Text,
            "a non-Base64 Data URL should remain ordinary text");
    require(!zeus::process_text(
                "data:text/plain;base64;charset=utf-8,SGVsbG8=",
                zeus::ProcessingMode::Base64).ok,
            "the Base64 marker must be the final Data URL metadata token");
    struct BinaryFixture {
        std::string bytes;
        const char* extension;
    };
    const std::vector<BinaryFixture> binary_fixtures{
        {std::string("\x89PNG\r\n\x1A\n", 8), ".png"},
        {std::string("\xFF\xD8\xFF\xE0", 4), ".jpg"},
        {"GIF89a", ".gif"},
        {std::string("RIFF\0\0\0\0WEBP", 12), ".webp"},
        {std::string("RIFF\0\0\0\0WAVE", 12), ".wav"},
        {"%PDF-1.7\n", ".pdf"},
        {std::string("PK\x03\x04", 4), ".zip"},
        {std::string("\x1F\x8B\x08\0", 4), ".gz"},
        {std::string("\x37\x7A\xBC\xAF\x27\x1C", 6), ".7z"},
        {std::string("Rar!\x1A\x07\0", 7), ".rar"},
        {std::string("SQLite format 3\0", 16), ".sqlite"},
    };
    for (const auto& fixture : binary_fixtures) {
        const auto encoded = zeus::process_text(
            fixture.bytes, zeus::ProcessingMode::Base64Encode);
        const auto decoded = zeus::process_text(
            encoded.value, zeus::ProcessingMode::Base64);
        require(decoded.binary_data && *decoded.binary_data == fixture.bytes &&
                    decoded.binary_extension == fixture.extension &&
                    decoded.value.find("Type: ") != std::string::npos &&
                    decoded.image_preview_source.empty(),
                "binary magic-byte detection should preserve bytes, type and extension");
    }

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
    require(detected_csv.ok && detected_csv.detected == zeus::ContentKind::Csv &&
                detected_csv.output_kind == zeus::ContentKind::Csv,
            "auto mode should detect consistent CSV content");

    const auto multiline_csv = zeus::parse_csv("id,note\n1,\"line one\nline two\"");
    require(multiline_csv.ok && multiline_csv.document.rows[1][1].find('\n') != std::string::npos,
            "CSV parser should preserve newlines inside quoted fields");

    require(zeus::process_text("<root><value>1</value></root>",
                zeus::ProcessingMode::Xml).detected == zeus::ContentKind::Xml,
            "registered XML formatter should execute through the core registry");
    require(zeus::process_text("name: Zeus\nenabled: true",
                zeus::ProcessingMode::Yaml).detected == zeus::ContentKind::Yaml,
            "registered YAML formatter should execute through the core registry");
    require(zeus::process_text("title = \"Zeus\"\n[window]\nwidth = 1200",
                zeus::ProcessingMode::Toml).detected == zeus::ContentKind::Toml,
            "registered TOML formatter should execute through the core registry");
    require(zeus::process_text("[window]\nwidth=1200\ntheme=system",
                zeus::ProcessingMode::Ini).detected == zeus::ContentKind::Ini,
            "registered INI formatter should execute through the core registry");
    require(zeus::process_text("name,value\nZeus,1",
                zeus::ProcessingMode::Csv).output_kind == zeus::ContentKind::Csv,
            "registered CSV processor should execute through the core registry");
    require(zeus::process_text("{\\\"name\\\":\\\"Zeus\\\"}",
                zeus::ProcessingMode::JsonUnescape).output_kind == zeus::ContentKind::Json,
            "registered JSON unescape processor should execute through the core registry");
    require(zeus::process_text("Zeus & Tools",
                zeus::ProcessingMode::HtmlEntityEncode).value == "Zeus &amp; Tools",
            "registered HTML Entity encoder should execute through the core registry");
    require(zeus::process_text("0x4869",
                zeus::ProcessingMode::HexDecode).value == "Hi",
            "registered Hex decoder should execute through the core registry");
    require(zeus::process_text("1786694400",
                zeus::ProcessingMode::Timestamp).ok,
            "registered timestamp inspector should execute through the core registry");

    std::set<std::string> processing_ids{"auto"};
    require(std::string(zeus::processing_mode_id(zeus::ProcessingMode::Auto)) == "auto",
            "automatic processing should expose its stable ID");
    for (int index = 1; index < static_cast<int>(zeus::ProcessingMode::Count); ++index) {
        const auto mode = static_cast<zeus::ProcessingMode>(index);
        const std::string id = zeus::processing_mode_id(mode);
        require(!id.empty(), "every explicit processing mode should expose a stable ID");
        require(processing_ids.insert(id).second,
                "processing mode IDs should remain globally unique");
    }

    const auto unsafe_yaml_candidate = zeus::process_text(
        "- &anchor value\n- *anchor");
    require(unsafe_yaml_candidate.ok &&
                unsafe_yaml_candidate.detected == zeus::ContentKind::Text,
            "failed conservative YAML candidates should continue through auto detection");

    std::cout << "All Zeus core tests passed\n";
    return 0;
}
