#include "app_controller.h"
#include "processing_registry.h"
#include "processing_service.h"
#include "../platform/file_dialog.h"

#include "components/input_model.h"
#include "core/platform/platform.h"
#include "core/window/window_backend.h"
#include "eui/app.h"
#include "eui/async.h"

#include "zeus/csv_document.h"
#include "zeus/secure_memory.h"
#include "zeus/json_formatter.h"
#include "zeus/locale_preference.h"
#include "zeus/system_locale.h"
#include "zeus/text_document.h"
#include "zeus/text_processor.h"
#include "zeus/text_selection.h"
#include "zeus/system_theme.h"
#include "zeus/theme_preference.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace app::controller {


using CsvCellMatch = zeus::CsvSearchMatch;
namespace {
AppState app_state_storage;
AppState& app_state = app_state_storage;

#ifdef ZEUS_DOCS_SCENARIO
std::string make_large_csv_scenario(std::size_t target_bytes) {
    std::string input = "id,name,city,status,notes\n";
    input.reserve(target_bytes);
    std::size_t row = 1;
    while (true) {
        const std::string value = std::to_string(row) +
            ",Zeus CSV benchmark,Shanghai,Active,searchable fixture\n";
        if (input.size() + value.size() + 256 > target_bytes) break;
        input += value;
        ++row;
    }
    const std::string final_prefix = std::to_string(row) +
        ",Zeus CSV final,Shanghai,Active,";
    input += final_prefix;
    input.append(target_bytes - input.size() - 1, 'x');
    input.push_back('\n');
    return input;
}
#endif
}

AppState& state() {
    return app_state_storage;
}

void initialize_documentation_scenario() {
#ifdef ZEUS_DOCS_SCENARIO
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    const std::string scenario(ZEUS_DOCS_SCENARIO);
    app_state.theme_preference_index = static_cast<int>(zeus::ThemePreference::Dark);
    app_state.locale_preference_index = static_cast<int>(zeus::LocalePreference::Chinese);

    if (scenario == "json") {
        app_state.input_text =
            R"({"service":"Zeus Tools","version":"0.2.1","offline":true,"formats":["JSON","XML","YAML","TOML","INI","CSV"],"features":{"autoDetect":true,"decodeDepth":1,"theme":"system"},"limits":{"maxInputMb":10},"releasedAt":"2026-08-21T08:00:00Z"})";
    } else if (scenario == "csv") {
        app_state.input_text =
            "id,name,format,status,latency_ms\n"
            "1001,Aurora,JSON,ready,4\n"
            "1002,Atlas,XML,ready,6\n"
            "1003,Nova,YAML,ready,5\n"
            "1004,Orion,CSV,ready,3\n"
            "1005,Helios,JWT,ready,4\n"
            "1006,Luna,Base64,ready,2";
        app_state.csv.delimiter_index = 1;
    } else if (scenario == "jwt") {
        app_state.input_text =
            "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
            "eyJzdWIiOiJkb2NzLXVzZXIiLCJuYW1lIjoiWmV1cyBEZW1vIiwicm9sZXMiOlsiZGV2ZWxvcGVyIiwicmV2aWV3ZXIiXSwib2ZmbGluZSI6dHJ1ZSwiaWF0IjoxNzg2Njk0NDAwfQ."
            "docs-signature";
    } else if (scenario == "image") {
        app_state.input_text =
            "data:image/png;base64,"
            "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAusB9Y9Zr6sAAAAASUVORK5CYII=";
    } else if (scenario == "toml") {
        app_state.input_text =
            "title = \"Zeus Tools\"\n"
            "version = \"0.2.1\"\n"
            "offline = true\n\n"
            "[window]\n"
            "width = 1280\n"
            "height = 860\n"
            "theme = \"system\"\n\n"
            "[formats]\n"
            "structured = [\"JSON\", \"XML\", \"YAML\", \"TOML\"]";
    } else if (scenario == "hmac") {
        app_state.input_text = "release=0.2.1&platform=desktop&offline=true";
        app_state.result.detected_input_kind = zeus::ContentKind::Text;
        app_state.result.output_kind = zeus::ContentKind::Text;
        app_state.crypto.panel_open = true;
        app_state.crypto.hmac = true;
        app_state.crypto.hmac_key = "docs-demo-key";
        app_state.crypto.key_visible = false;
        compute_crypto_output(zeus::DigestAlgorithm::Sha256);
    } else if (scenario == "crc32") {
        app_state.input_text = "Zeus Tools CRC32 verification";
        app_state.result.detected_input_kind = zeus::ContentKind::Text;
        app_state.result.output_kind = zeus::ContentKind::Text;
        app_state.crypto.panel_open = true;
        compute_crypto_output(zeus::DigestAlgorithm::Crc32);
    } else if (scenario == "about") {
        app_state.about_dialog_open = true;
    } else if (scenario == "oversized") {
        app_state.input_text.assign(processing::kRecommendedMaxInputBytes + 1, 'x');
    } else if (scenario == "large-json") {
        constexpr std::string_view prefix = "{\"payload\":\"";
        constexpr std::string_view suffix = "\"}";
        app_state.input_text.assign(prefix);
        app_state.input_text.append(
            processing::kRecommendedMaxInputBytes - prefix.size() - suffix.size(), 'x');
        app_state.input_text.append(suffix);
    } else if (scenario == "large-csv") {
        app_state.input_text = make_large_csv_scenario(
            processing::kRecommendedMaxInputBytes);
        app_state.csv.delimiter_index = 1;
    }
#endif
}

zeus::LocalePreference system_locale_preference() {
    std::string tag = zeus::system_locale_tag();
    std::transform(tag.begin(), tag.end(), tag.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (tag.rfind("zh-hant", 0) == 0 || tag.rfind("zh-tw", 0) == 0 ||
        tag.rfind("zh-hk", 0) == 0 || tag.rfind("zh-mo", 0) == 0) {
        return zeus::LocalePreference::TraditionalChinese;
    }
    if (tag.rfind("zh", 0) == 0) return zeus::LocalePreference::Chinese;
    if (tag.rfind("ja", 0) == 0) return zeus::LocalePreference::Japanese;
    if (tag.rfind("ko", 0) == 0) return zeus::LocalePreference::Korean;
    if (tag.rfind("es", 0) == 0) return zeus::LocalePreference::Spanish;
    if (tag.rfind("fr", 0) == 0) return zeus::LocalePreference::French;
    if (tag.rfind("de", 0) == 0) return zeus::LocalePreference::German;
    if (tag.rfind("pt", 0) == 0) return zeus::LocalePreference::Portuguese;
    if (tag.rfind("ru", 0) == 0) return zeus::LocalePreference::Russian;
    return zeus::LocalePreference::English;
}

zeus::LocalePreference effective_locale() {
    const auto preference = static_cast<zeus::LocalePreference>(app_state.locale_preference_index);
    return preference == zeus::LocalePreference::System
        ? system_locale_preference() : preference;
}

const char* tr(i18n::Text text) {
    return i18n::get(text, effective_locale());
}

void request_full_repaint() {
    ++app_state.full_repaint_revision;
    // EUI renders through a retained off-screen cache. A regular UI update
    // only recomposes the element tree and repaints its calculated dirty
    // rectangles, which can leave pixels from a dismissed popup behind when
    // its old bounds are no longer present in the new tree. Force a complete
    // cache clear and paint for state changes that can alter layered content.
    ::app::detail::requestFullPaint();
}

const char* theme_label() {
    switch (static_cast<zeus::ThemePreference>(app_state.theme_preference_index)) {
    case zeus::ThemePreference::Light: return tr(i18n::Text::Light);
    case zeus::ThemePreference::Dark: return tr(i18n::Text::Dark);
    case zeus::ThemePreference::System: return tr(i18n::Text::System);
    }
    return tr(i18n::Text::System);
}

unsigned int theme_icon() {
    switch (static_cast<zeus::ThemePreference>(app_state.theme_preference_index)) {
    case zeus::ThemePreference::Light: return 0xF185;
    case zeus::ThemePreference::Dark: return 0xF186;
    case zeus::ThemePreference::System: return 0xF108;
    }
    return 0xF108;
}

std::string theme_tooltip() {
    return std::string(tr(i18n::Text::Theme)) + ": " + theme_label();
}

std::string compact_issue(std::string value) {
    const auto line_break = value.find_first_of("\r\n");
    if (line_break != std::string::npos) value.resize(line_break);
    constexpr std::size_t max_bytes = 56;
    if (value.size() <= max_bytes) return value;
    std::size_t end = max_bytes;
    while (end > 0 &&
           (static_cast<unsigned char>(value[end]) & 0xC0U) == 0x80U) {
        --end;
    }
    value.resize(end);
    value += "…";
    return value;
}

std::vector<std::string> language_items() {
    return {tr(i18n::Text::System), "简体中文", "English", "繁體中文", "日本語",
            "한국어", "Español", "Français", "Deutsch", "Português", "Русский"};
}

bool use_dark_theme() {
    if (app_state.theme_preference_index == static_cast<int>(zeus::ThemePreference::Light)) return false;
    if (app_state.theme_preference_index == static_cast<int>(zeus::ThemePreference::Dark)) return true;
    return zeus::system_prefers_dark();
}

std::vector<std::string> input_type_items() {
    std::vector<std::string> items;
    for (const auto& type : processing::registered_input_types()) {
        items.emplace_back(type.id == "auto"
            ? tr(i18n::Text::Auto)
            : processing::content_definition(type.kind).label);
    }
    return items;
}

std::vector<std::string> csv_delimiter_items() {
    return {tr(i18n::Text::DelimiterAuto), tr(i18n::Text::DelimiterComma), "Tab",
            tr(i18n::Text::DelimiterSemicolon), tr(i18n::Text::DelimiterPipe)};
}

std::vector<std::string> crypto_message_items() {
    return {tr(i18n::Text::MessageInput), tr(i18n::Text::MessageResult)};
}

std::vector<std::string> hmac_key_encoding_items() {
    return {"UTF-8", "Hex", "Base64"};
}

zeus::HmacKeyEncoding hmac_key_encoding() {
    switch (app_state.crypto.key_encoding_index) {
    case 1: return zeus::HmacKeyEncoding::Hex;
    case 2: return zeus::HmacKeyEncoding::Base64;
    default: return zeus::HmacKeyEncoding::Utf8;
    }
}

void clear_hmac_key() {
    zeus::secure_clear(app_state.crypto.hmac_key);
    app_state.crypto.key_visible = false;
    app_state.crypto.message_source_index = 0;
    app_state.crypto.message_dropdown_open = false;
    app_state.crypto.key_encoding_index = 0;
    app_state.crypto.key_encoding_dropdown_open = false;
}

void set_hmac_key(std::string_view value) {
    zeus::secure_clear(app_state.crypto.hmac_key);
    app_state.crypto.hmac_key.assign(value.data(), value.size());
}

void clear_hmac_input_state(eui::Ui& ui) {
    using InputState = components::input_detail::InputModel::InputState;
    InputState& state = ui.state<InputState>("actions.crypto.key");
    zeus::secure_clear(state.text);
    zeus::secure_clear(state.compositionText);
    for (auto& snapshot : state.undoStack) zeus::secure_clear(snapshot.text);
    for (auto& snapshot : state.redoStack) zeus::secure_clear(snapshot.text);
    state.undoStack.clear();
    state.redoStack.clear();
    ui.releaseStateScope("actions.crypto.key");
}

float csv_match_vertical_offset(std::size_t row) {
    const std::size_t visible_row = app_state.csv.first_row_header && row > 0 ? row - 1 : row;
    return static_cast<float>(visible_row) * 28.0f;
}

struct SearchPayload {
    std::string query;
    bool case_sensitive = false;
    bool use_regex = false;
    std::string error;
    std::shared_ptr<zeus::HighlightedDocument> document_source;
    std::shared_ptr<zeus::CsvDocument> csv_source;
    std::vector<zeus::SearchMatch> document_matches;
    std::vector<CsvCellMatch> csv_matches;
};

components::theme::ThemeColorTokens theme_tokens(bool dark) {
    auto tokens = dark ? components::theme::dark() : components::theme::light();
    tokens.primary = {0.36f, 0.66f, 1.0f, 1.0f};
    return tokens;
}

void reset_result_interaction_state() {
    app_state.result.folds.clear();
    app_state.result.scroll = 0.0f;
    app_state.result.csv_horizontal_scroll = 0.0f;
    app_state.result.selected_csv_row = kNoCsvCell;
    app_state.result.selected_csv_column = kNoCsvCell;
    app_state.result.selection.clear();
}

static void report_operation_failure(const char* summary, const char* detail) {
    app_state.result.status = tr(i18n::Text::Invalid);
    app_state.result.issue = summary;
    app_state.result.issue_detail = detail == nullptr || detail[0] == '\0'
        ? "The operation could not be completed" : detail;
    request_full_repaint();
}

void reveal_result_line(std::size_t line) {
    if (!app_state.result.document) return;
    app_state.result.folds.ensure_document(*app_state.result.document);
    app_state.result.folds.reveal(*app_state.result.document, line);
    app_state.result.scroll = static_cast<float>(
        app_state.result.folds.visible_index(*app_state.result.document, line)) * 25.0f;
}

void update_search() {
    app_state.search.active_match = 0;
    app_state.search.document_matches.clear();
    app_state.search.csv_matches.clear();
    app_state.search.issue.clear();
    app_state.search.issue_detail.clear();
    if (app_state.search.query.empty()) return;

    const std::string query = app_state.search.query;
    const bool case_sensitive = app_state.search.case_sensitive;
    const bool use_regex = app_state.search.use_regex;
    const auto document = app_state.result.document;
    const auto csv = app_state.result.csv;
    async::restart(
        "zeus.search.result",
        [query, case_sensitive, use_regex, document, csv](const async::CancelToken& token) {
            SearchPayload payload;
            payload.query = query;
            payload.case_sensitive = case_sensitive;
            payload.use_regex = use_regex;
            payload.document_source = document;
            payload.csv_source = csv;
            try {
                for (int elapsed = 0; elapsed < 100 && !token.canceled(); elapsed += 20) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
                if (token.canceled()) return async::success(std::move(payload));
                if (csv) {
                    payload.csv_matches = csv->search(
                        query, case_sensitive, use_regex, &payload.error);
                } else if (document) {
                    payload.document_matches = document->search(
                        query, case_sensitive, use_regex, &payload.error);
                }
                return async::success(std::move(payload));
            } catch (const std::exception& exception) {
                return async::failure<SearchPayload>(exception.what());
            } catch (...) {
                return async::failure<SearchPayload>("Unknown background search error");
            }
        },
        [](const async::Result<SearchPayload>& completed) {
            if (!completed.ok) {
                app_state.search.issue = tr(i18n::Text::Invalid);
                app_state.search.issue_detail = completed.error.empty()
                    ? "Background search failed" : completed.error;
                core::platform::requestUiUpdate();
                return;
            }
            if (completed.value.query != app_state.search.query ||
                completed.value.case_sensitive != app_state.search.case_sensitive ||
                completed.value.use_regex != app_state.search.use_regex) return;
            const auto& payload = completed.value;
            if (payload.csv_source != app_state.result.csv ||
                payload.document_source != app_state.result.document) return;
            app_state.search.active_match = 0;
            app_state.search.issue = payload.error.empty()
                ? std::string{}
                : tr(i18n::Text::InvalidRegex);
            app_state.search.issue_detail = payload.error;
            app_state.search.document_matches = payload.document_matches;
            app_state.search.csv_matches = payload.csv_matches;
            if (!app_state.search.csv_matches.empty()) {
                const auto& first = app_state.search.csv_matches.front();
                app_state.result.scroll = csv_match_vertical_offset(first.row);
                app_state.result.csv_horizontal_scroll = static_cast<float>(first.column) * 180.0f;
            } else if (!app_state.search.document_matches.empty()) {
                reveal_result_line(app_state.search.document_matches.front().line);
            }
            core::platform::requestUiUpdate();
        });
}

void analyze_input(bool debounce) {
    if (app_state.input_text.empty()) {
        app_state.oversized_input_approved = false;
        async::cancel("zeus.process.input");
        async::cancel("zeus.search.result");
        reset_result_interaction_state();
        app_state.result.detected_input_kind = zeus::ContentKind::Text;
        app_state.result.output_kind = zeus::ContentKind::Text;
        app_state.result.document = std::make_shared<zeus::HighlightedDocument>(
            zeus::HighlightedDocument::plain(""));
        app_state.result.csv.reset();
        app_state.result.binary_data.reset();
        app_state.result.binary_extension.clear();
        app_state.result.image_preview_source.clear();
        app_state.result.decode_chain.clear();
        app_state.result.can_continue_decode = false;
        app_state.result.status = tr(i18n::Text::WaitingForInput);
        app_state.result.issue.clear();
        app_state.result.issue_detail.clear();
        app_state.search.active_match = 0;
        app_state.search.document_matches.clear();
        app_state.search.csv_matches.clear();
        app_state.search.issue.clear();
        app_state.search.issue_detail.clear();
        request_full_repaint();
        return;
    }

    if (!processing::exceeds_recommended_input_size(app_state.input_text.size())) {
        app_state.oversized_input_approved = false;
    } else if (!app_state.oversized_input_approved) {
        async::cancel("zeus.process.input");
        async::cancel("zeus.search.result");
        reset_result_interaction_state();
        app_state.result.detected_input_kind = zeus::ContentKind::Text;
        app_state.result.output_kind = zeus::ContentKind::Text;
        app_state.result.document = std::make_shared<zeus::HighlightedDocument>(
            zeus::HighlightedDocument::plain(""));
        app_state.result.csv.reset();
        app_state.result.binary_data.reset();
        app_state.result.binary_extension.clear();
        app_state.result.image_preview_source.clear();
        app_state.result.decode_chain.clear();
        app_state.result.can_continue_decode = false;
        app_state.result.status = tr(i18n::Text::Invalid);
        app_state.result.issue = tr(i18n::Text::InputTooLarge);
        app_state.result.issue_detail = app_state.result.issue;
        app_state.search.active_match = 0;
        app_state.search.document_matches.clear();
        app_state.search.csv_matches.clear();
        app_state.search.issue.clear();
        app_state.search.issue_detail.clear();
        request_full_repaint();
        return;
    }

    processing::AnalysisRequest request;
    request.input = app_state.input_text;
    request.action_id = app_state.processing_action_id;
    request.input_type_id = app_state.input_type_id;
    request.csv_delimiter_index = app_state.csv.delimiter_index;
    request.first_row_header = app_state.csv.first_row_header;

    app_state.result.status = debounce ? tr(i18n::Text::WaitingForInput) : tr(i18n::Text::Processing);
    app_state.result.issue.clear();
    app_state.result.issue_detail.clear();
    core::platform::requestUiUpdate();

    async::restart(
        "zeus.process.input",
        [request = std::move(request), debounce](const async::CancelToken& token) {
            try {
                if (debounce) {
                    for (int elapsed = 0; elapsed < 180 && !token.canceled(); elapsed += 20) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    }
                }
                if (token.canceled()) {
                    return async::success(processing::AnalysisResult{});
                }
                auto result = processing::analyze(request);
                return async::success(token.canceled()
                    ? processing::AnalysisResult{} : std::move(result));
            } catch (const std::exception& exception) {
                return async::failure<processing::AnalysisResult>(exception.what());
            } catch (...) {
                return async::failure<processing::AnalysisResult>(
                    "Unknown background processing error");
            }
        },
        [](const async::Result<processing::AnalysisResult>& completed) {
            if (!completed.ok) {
                app_state.result.issue = tr(i18n::Text::Invalid);
                app_state.result.issue_detail = completed.error.empty()
                    ? "Background processing failed" : completed.error;
                app_state.result.status = tr(i18n::Text::Invalid);
                request_full_repaint();
                return;
            }
            if (!completed.value.document && !completed.value.csv) return;
            const processing::AnalysisResult& payload = completed.value;
            app_state.result.detected_input_kind = payload.detected;
            app_state.result.output_kind = payload.process.output_kind;
            reset_result_interaction_state();
            app_state.result.document = payload.document;
            app_state.result.csv = payload.csv;
            app_state.result.binary_data = payload.process.binary_data;
            app_state.result.binary_extension = payload.process.binary_extension;
            app_state.result.image_preview_source = payload.process.image_preview_source;
            const auto& result = payload.process;
            app_state.result.decode_chain = payload.decode_chain;
            app_state.result.can_continue_decode = payload.can_continue_decode;
            if (!result.ok) {
                app_state.result.issue = compact_issue(
                    result.error_message.empty() ? result.error_code : result.error_message);
                app_state.result.issue_detail = result.error_message.empty()
                    ? result.error_code : result.error_message;
                if (!result.error_code.empty() && result.error_code != app_state.result.issue_detail) {
                    app_state.result.issue_detail += "\nCode: " + result.error_code;
                }
                if (result.error_line != 0) {
                    app_state.result.issue += " · L" + std::to_string(result.error_line) +
                                  ":C" + std::to_string(result.error_column);
                    app_state.result.issue_detail += "\nL" + std::to_string(result.error_line) +
                                         ":C" + std::to_string(result.error_column);
                }
                app_state.result.status = std::string(tr(i18n::Text::Invalid)) + " " + result.label;
            } else if (payload.csv) {
                app_state.result.issue.clear();
                app_state.result.issue_detail.clear();
                const std::size_t data_rows = payload.csv->rows.size() -
                    (payload.first_row_header && !payload.csv->rows.empty() ? 1 : 0);
                app_state.result.status = "CSV · " + std::to_string(data_rows) + " " +
                              tr(i18n::Text::Rows) + " · " +
                              std::to_string(payload.elapsed_ms) + " ms";
            } else {
                app_state.result.issue.clear();
                app_state.result.issue_detail.clear();
                const std::size_t result_bytes = result.binary_data
                    ? result.binary_data->size() : result.value.size();
                app_state.result.status = result.label + " · " + std::to_string(result_bytes) +
                              " " + tr(i18n::Text::Bytes) + " · " +
                              std::to_string(payload.elapsed_ms) + " ms";
            }
            update_search();
            request_full_repaint();
        });
}

bool oversized_input_paused() {
    return oversized_input() &&
        !app_state.oversized_input_approved;
}

bool oversized_input() {
    return processing::exceeds_recommended_input_size(app_state.input_text.size());
}

bool lightweight_input_preview() {
    return processing::requires_lightweight_input_preview(app_state.input_text.size());
}

void process_oversized_input() {
    if (!processing::exceeds_recommended_input_size(app_state.input_text.size())) return;
    app_state.oversized_input_approved = true;
    request_full_repaint();
    analyze_input(false);
}

void continue_decode_one_layer() {
    if (!app_state.result.can_continue_decode || !app_state.result.document) return;
    const std::string source = app_state.result.document->text();
    const std::string previous_chain = app_state.result.decode_chain;
    app_state.result.status = tr(i18n::Text::Processing);
    app_state.result.issue.clear();
    app_state.result.issue_detail.clear();
    core::platform::requestUiUpdate();

    async::restart(
        "zeus.process.input",
        [source, previous_chain](const async::CancelToken& token) {
            try {
                if (token.canceled()) {
                    return async::success(processing::DecodeResult{});
                }
                auto result = processing::decode_one_layer(source, previous_chain);
                return async::success(token.canceled()
                    ? processing::DecodeResult{} : std::move(result));
            } catch (const std::exception& exception) {
                return async::failure<processing::DecodeResult>(exception.what());
            } catch (...) {
                return async::failure<processing::DecodeResult>(
                    "Unknown background decoding error");
            }
        },
        [](const async::Result<processing::DecodeResult>& completed) {
            if (!completed.ok) {
                app_state.result.issue = tr(i18n::Text::Invalid);
                app_state.result.issue_detail = completed.error.empty()
                    ? "Background decoding failed" : completed.error;
                app_state.result.status = tr(i18n::Text::Invalid);
                request_full_repaint();
                return;
            }
            if (!app_state.result.document ||
                completed.value.source != app_state.result.document->text()) return;
            const processing::DecodeResult& payload = completed.value;
            if (!payload.process.ok || !payload.document) {
                app_state.result.issue = compact_issue(payload.process.error_message);
                app_state.result.issue_detail = payload.process.error_message;
                app_state.result.status = tr(i18n::Text::Invalid);
                app_state.result.can_continue_decode = false;
                core::platform::requestUiUpdate();
                return;
            }
            app_state.result.output_kind = payload.process.output_kind;
            app_state.result.decode_chain = payload.chain;
            app_state.result.can_continue_decode = payload.can_continue;
            reset_result_interaction_state();
            app_state.result.document = payload.document;
            app_state.result.csv.reset();
            app_state.result.binary_data = payload.process.binary_data;
            app_state.result.binary_extension = payload.process.binary_extension;
            app_state.result.image_preview_source = payload.process.image_preview_source;
            app_state.result.issue.clear();
            app_state.result.issue_detail.clear();
            app_state.result.status = app_state.result.decode_chain + " · " +
                std::to_string(payload.process.value.size()) + " " +
                tr(i18n::Text::Bytes);
            update_search();
            request_full_repaint();
        });
}

void move_match(int direction) {
    const std::size_t match_count = app_state.result.csv
        ? app_state.search.csv_matches.size()
        : app_state.search.document_matches.size();
    if (match_count == 0) {
        return;
    }
    const std::int64_t count = static_cast<std::int64_t>(match_count);
    const std::int64_t current = static_cast<std::int64_t>(app_state.search.active_match);
    app_state.search.active_match = static_cast<std::size_t>((current + direction + count) % count);
    if (app_state.result.csv) {
        const auto& match = app_state.search.csv_matches[app_state.search.active_match];
        app_state.result.scroll = csv_match_vertical_offset(match.row);
        app_state.result.csv_horizontal_scroll = static_cast<float>(match.column) * 180.0f;
    } else {
        reveal_result_line(app_state.search.document_matches[app_state.search.active_match].line);
    }
    core::platform::requestUiUpdate();
}

static void copy_result_impl(bool selection_only) {
    if (app_state.result.csv) {
        const bool selected = app_state.result.selected_csv_row < app_state.result.csv->rows.size() &&
            app_state.result.selected_csv_column < app_state.result.csv->rows[app_state.result.selected_csv_row].size();
        if (selection_only && !selected) return;
        std::string table_value;
        const std::string* value = nullptr;
        if (selected) {
            value = &app_state.result.csv->rows[app_state.result.selected_csv_row]
                [app_state.result.selected_csv_column];
        } else {
            table_value = app_state.result.csv->to_tsv();
            value = &table_value;
        }
        if (value->empty() && selection_only) return;
        core::window::setClipboardText(*value);
        app_state.result.status = std::string(tr(i18n::Text::Copied)) + " " +
                      std::to_string(value->size()) + " " + tr(i18n::Text::Bytes);
        core::platform::requestUiUpdate();
        return;
    }
    if (!app_state.result.document) {
        return;
    }
    std::string selection =
        app_state.result.selection.selected_text(*app_state.result.document);
    const std::string* value = selection.empty() && !selection_only
        ? &app_state.result.document->text() : &selection;
    if (value->empty()) return;
    core::window::setClipboardText(*value);
    app_state.result.status = std::string(tr(i18n::Text::Copied)) + " " +
                  std::to_string(value->size()) + " " + tr(i18n::Text::Bytes);
    core::platform::requestUiUpdate();
}

void copy_result(bool selection_only) {
    try {
        copy_result_impl(selection_only);
    } catch (const std::exception& exception) {
        report_operation_failure("Unable to copy result", exception.what());
    } catch (...) {
        report_operation_failure("Unable to copy result", "Unknown clipboard error");
    }
}

void copy_input_text() {
    if (app_state.input_text.empty()) return;
    try {
        core::window::setClipboardText(app_state.input_text);
        app_state.result.status = std::string(tr(i18n::Text::Copied)) + " " +
            std::to_string(app_state.input_text.size()) + " " + tr(i18n::Text::Bytes);
        core::platform::requestUiUpdate();
    } catch (const std::exception& exception) {
        report_operation_failure("Unable to copy input", exception.what());
    } catch (...) {
        report_operation_failure("Unable to copy input", "Unknown clipboard error");
    }
}

static void load_input_file_impl(const std::string& path) {
    if (path.empty()) return;
    constexpr std::uintmax_t max_file_bytes = 10U * 1024U * 1024U;
    std::error_code size_error;
    const std::uintmax_t size = std::filesystem::file_size(path, size_error);
    if (size_error || size > max_file_bytes) {
        app_state.result.status = tr(i18n::Text::Invalid);
        app_state.result.issue = size_error ? "File unavailable" : "File exceeds 10 MB";
        app_state.result.issue_detail = size_error.message();
        request_full_repaint();
        return;
    }

    std::ifstream input(path, std::ios::binary);
    std::string value(static_cast<std::size_t>(size), '\0');
    if (!input || (size != 0 && !input.read(value.data(), static_cast<std::streamsize>(size)))) {
        app_state.result.status = tr(i18n::Text::Invalid);
        app_state.result.issue = "Unable to read file";
        app_state.result.issue_detail = "The selected file could not be read";
        request_full_repaint();
        return;
    }
    if (value.size() >= 3 && static_cast<unsigned char>(value[0]) == 0xEFU &&
        static_cast<unsigned char>(value[1]) == 0xBBU &&
        static_cast<unsigned char>(value[2]) == 0xBFU) {
        value.erase(0, 3);
    }
    if (value.find('\0') != std::string::npos) {
        app_state.result.status = tr(i18n::Text::Invalid);
        app_state.result.issue = "Binary file is not supported";
        app_state.result.issue_detail = "Open a UTF-8 text or structured-data file";
        request_full_repaint();
        return;
    }

    app_state.input_text = std::move(value);
    app_state.oversized_input_approved = false;
    app_state.large_input_page = 0;
    app_state.large_input_page_boundaries.clear();
    app_state.processing_action_id = "auto";
    app_state.input_type_id = "auto";
    app_state.input_type_dropdown_open = false;
    app_state.csv.delimiter_index = 0;
    app_state.result.decode_chain.clear();
    app_state.result.can_continue_decode = false;
    analyze_input(false);
}

void load_input_file(const std::string& path) {
    try {
        load_input_file_impl(path);
    } catch (const std::exception& exception) {
        report_operation_failure("Unable to read file", exception.what());
    } catch (...) {
        report_operation_failure("Unable to read file", "Unknown file read error");
    }
}

void open_input_file() {
    try {
        load_input_file(platform::choose_input_file());
    } catch (const std::exception& exception) {
        report_operation_failure("Unable to open file", exception.what());
    } catch (...) {
        report_operation_failure("Unable to open file", "Unknown file dialog error");
    }
}

static void export_result_impl() {
    if (!app_state.result.document && !app_state.result.csv &&
        !app_state.result.binary_data) return;
    const bool binary = static_cast<bool>(app_state.result.binary_data);
    const std::string extension = binary
        ? (app_state.result.binary_extension.empty()
              ? ".bin" : app_state.result.binary_extension)
        : std::string(processing::content_definition(
              app_state.result.output_kind).export_extension);
    const std::string path = platform::choose_export_file(
        binary ? "zeus-decoded" + extension : "zeus-result" + extension);
    if (path.empty()) return;
    std::string table_value;
    const std::string* value = nullptr;
    if (binary) {
        value = app_state.result.binary_data.get();
    } else if (app_state.result.csv) {
        table_value = app_state.result.csv->to_tsv();
        value = &table_value;
    } else {
        value = &app_state.result.document->text();
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(value->data(), static_cast<std::streamsize>(value->size()));
    if (!output) {
        app_state.result.status = tr(i18n::Text::Invalid);
        app_state.result.issue = "Unable to export result";
        app_state.result.issue_detail = "The selected file could not be written";
    } else {
        app_state.result.issue.clear();
        app_state.result.issue_detail.clear();
        app_state.result.status = std::string(tr(binary
                ? i18n::Text::ExportBinary : i18n::Text::ExportResult)) + " · " +
            std::to_string(value->size()) + " " + tr(i18n::Text::Bytes);
    }
    request_full_repaint();
}

void export_result() {
    try {
        export_result_impl();
    } catch (const std::exception& exception) {
        report_operation_failure("Unable to export result", exception.what());
    } catch (...) {
        report_operation_failure("Unable to export result", "Unknown file write error");
    }
}

void compute_crypto_output(zeus::DigestAlgorithm algorithm) {
    const bool use_result = app_state.crypto.message_source_index == 1 &&
        (app_state.result.document || app_state.result.csv);
    std::string csv_message;
    const std::string* message = &app_state.input_text;
    if (use_result && app_state.result.binary_data) {
        message = app_state.result.binary_data.get();
    } else if (use_result && app_state.result.document) {
        message = &app_state.result.document->text();
    } else if (use_result && app_state.result.csv) {
        csv_message = app_state.result.csv->to_tsv();
        message = &csv_message;
    }
    const std::string name = app_state.crypto.hmac
        ? "HMAC-" + std::string(zeus::digest_algorithm_name(algorithm))
        : zeus::digest_algorithm_name(algorithm);
    zeus::CryptoResult result;
    try {
        result = app_state.crypto.hmac
            ? zeus::compute_hmac_encoded(
                  *message, app_state.crypto.hmac_key, hmac_key_encoding(), algorithm)
            : zeus::compute_digest(*message, algorithm);
    } catch (const std::exception& exception) {
        app_state.result.issue = tr(i18n::Text::Invalid);
        app_state.result.issue_detail = exception.what();
        app_state.result.status = name;
        request_full_repaint();
        return;
    } catch (...) {
        app_state.result.issue = tr(i18n::Text::Invalid);
        app_state.result.issue_detail = "Unknown crypto processing error";
        app_state.result.status = name;
        request_full_repaint();
        return;
    }
    if (!result.ok) {
        app_state.result.issue = compact_issue(result.error);
        app_state.result.issue_detail = result.error;
        app_state.result.status = name;
        core::platform::requestUiUpdate();
        return;
    }
    std::string display = name + "\n\nHex\n" + result.hex +
                          "\n\nBase64\n" + result.base64;
    if (zeus::digest_algorithm_is_weak(algorithm)) {
        display += "\n\n⚠ " + std::string(tr(i18n::Text::WeakAlgorithm));
    } else if (zeus::digest_algorithm_is_checksum(algorithm)) {
        display += "\n\n⚠ " + std::string(tr(i18n::Text::ChecksumOnly));
    }
    reset_result_interaction_state();
    app_state.result.document = std::make_shared<zeus::HighlightedDocument>(
        zeus::HighlightedDocument::plain(std::move(display)));
    app_state.result.csv.reset();
    app_state.result.binary_data.reset();
    app_state.result.binary_extension.clear();
    app_state.result.image_preview_source.clear();
    app_state.result.can_continue_decode = false;
    app_state.result.decode_chain.clear();
    app_state.result.issue.clear();
    app_state.result.issue_detail.clear();
    app_state.result.status = name + " · " +
        (use_result ? tr(i18n::Text::MessageResult) : tr(i18n::Text::MessageInput)) +
        " · Hex + Base64";
    core::platform::requestUiUpdate();
}


} // namespace app::controller
