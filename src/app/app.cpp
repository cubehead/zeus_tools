#include "eui_neo.h"

#include "i18n.h"
#include "csv_table_view.h"
#include "font_tokens.h"
#include "rich_text_view.h"

#include "zeus/csv_document.h"
#include "zeus/crypto_service.h"
#include "zeus/json_formatter.h"
#include "zeus/locale_preference.h"
#include "zeus/system_locale.h"
#include "zeus/system_ui_font.h"
#include "zeus/text_document.h"
#include "zeus/text_processor.h"
#include "zeus/text_selection.h"
#include "zeus/system_theme.h"
#include "zeus/theme_preference.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace app {

const DslAppConfig& dslAppConfig() {
    const char* ui_font = zeus::system_ui_font_path();
    if (ui_font != nullptr && ui_font[0] != '\0') {
        core::TextPrimitive::setDefaultFontFiles(ui_font, "");
    }
    static const DslAppConfig config = DslAppConfig{}
        .title("Zeus Tools")
        .pageId("zeus_tools")
        .iconPath("assets/zeus-tools-1024.png")
        .clearColor({0.055f, 0.063f, 0.078f, 1.0f})
        .windowSize(1280, 860)
        .fps(90.0);
    return config;
}

namespace {

std::string input_text = R"({"name":"Zeus Tools","offline":true,"formats":["JSON","XML","YAML","CSV"],"limits":{"maxInputMb":10,"autoDecodeDepth":1}})";
std::string search_query;
std::string status_text = "JSON · Ready";
std::string issue_text;
std::string issue_detail_text;
std::string search_issue_text;
std::string search_issue_detail_text;
bool search_case_sensitive = false;
bool search_use_regex = false;
float result_scroll = 0.0f;
float csv_horizontal_scroll = 0.0f;
std::size_t active_match = 0;
std::size_t full_repaint_revision = 0;
std::vector<zeus::SearchMatch> search_matches;
using CsvCellMatch = zeus::CsvSearchMatch;
std::vector<CsvCellMatch> csv_search_matches;
constexpr std::size_t no_csv_cell = std::numeric_limits<std::size_t>::max();
std::size_t selected_csv_row = no_csv_cell;
std::size_t selected_csv_column = no_csv_cell;
std::shared_ptr<zeus::HighlightedDocument> result_document;
std::shared_ptr<zeus::CsvDocument> result_csv;
zeus::TextSelection result_selection;
zeus::TextFoldState result_folds;
int processing_mode_index = 0;
int input_type_override_index = 0;
bool input_type_dropdown_open = false;
int csv_delimiter_index = 0;
bool csv_delimiter_dropdown_open = false;
bool csv_first_row_header = true;
bool language_dropdown_open = false;
bool crypto_panel_open = false;
bool crypto_hmac = false;
std::string hmac_key;
zeus::ContentKind detected_input_kind = zeus::ContentKind::Text;
int theme_preference_index = static_cast<int>(zeus::load_theme_preference());
int locale_preference_index = static_cast<int>(zeus::load_locale_preference());

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
    const auto preference = static_cast<zeus::LocalePreference>(locale_preference_index);
    return preference == zeus::LocalePreference::System
        ? system_locale_preference() : preference;
}

const char* tr(i18n::Text text) {
    return i18n::get(text, effective_locale());
}

void request_full_repaint() {
    ++full_repaint_revision;
    core::platform::requestUiUpdate();
}

const char* theme_label() {
    switch (static_cast<zeus::ThemePreference>(theme_preference_index)) {
    case zeus::ThemePreference::Light: return tr(i18n::Text::Light);
    case zeus::ThemePreference::Dark: return tr(i18n::Text::Dark);
    case zeus::ThemePreference::System: return tr(i18n::Text::System);
    }
    return tr(i18n::Text::System);
}

unsigned int theme_icon() {
    switch (static_cast<zeus::ThemePreference>(theme_preference_index)) {
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
    if (theme_preference_index == static_cast<int>(zeus::ThemePreference::Light)) return false;
    if (theme_preference_index == static_cast<int>(zeus::ThemePreference::Dark)) return true;
    return zeus::system_prefers_dark();
}

zeus::ProcessingMode processing_mode_for(int mode_index, zeus::ContentKind detected_kind) {
    switch (mode_index) {
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

std::vector<std::string> input_type_items() {
    return {tr(i18n::Text::Auto), "JSON", "XML", "YAML", "CSV", "Base64", "URL", "Text"};
}

std::vector<std::string> csv_delimiter_items() {
    return {tr(i18n::Text::DelimiterAuto), tr(i18n::Text::DelimiterComma), "Tab",
            tr(i18n::Text::DelimiterSemicolon), tr(i18n::Text::DelimiterPipe)};
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

float csv_match_vertical_offset(std::size_t row) {
    const std::size_t visible_row = csv_first_row_header && row > 0 ? row - 1 : row;
    return static_cast<float>(visible_row) * 28.0f;
}

struct AnalysisPayload {
    zeus::ProcessResult result;
    zeus::ContentKind detected = zeus::ContentKind::Text;
    std::shared_ptr<zeus::HighlightedDocument> document;
    std::shared_ptr<zeus::CsvDocument> csv;
    bool first_row_header = true;
    std::int64_t elapsed_ms = 0;
};

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

void reveal_result_line(std::size_t line) {
    if (!result_document) return;
    result_folds.ensure_document(*result_document);
    result_folds.reveal(*result_document, line);
    result_scroll = static_cast<float>(result_folds.visible_index(*result_document, line)) * 25.0f;
}

void update_search() {
    active_match = 0;
    search_matches.clear();
    csv_search_matches.clear();
    search_issue_text.clear();
    search_issue_detail_text.clear();
    if (search_query.empty()) return;

    const std::string query = search_query;
    const bool case_sensitive = search_case_sensitive;
    const bool use_regex = search_use_regex;
    const auto document = result_document;
    const auto csv = result_csv;
    async::restart(
        "zeus.search.result",
        [query, case_sensitive, use_regex, document, csv](const async::CancelToken& token) {
            for (int elapsed = 0; elapsed < 100 && !token.canceled(); elapsed += 20) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            SearchPayload payload;
            payload.query = query;
            payload.case_sensitive = case_sensitive;
            payload.use_regex = use_regex;
            payload.document_source = document;
            payload.csv_source = csv;
            if (token.canceled()) return payload;
            if (csv) {
                payload.csv_matches = csv->search(
                    query, case_sensitive, use_regex, &payload.error);
            } else if (document) {
                payload.document_matches = document->search(
                    query, case_sensitive, use_regex, &payload.error);
            }
            return payload;
        },
        [](const async::Result<SearchPayload>& completed) {
            if (!completed.ok || completed.value.query != search_query ||
                completed.value.case_sensitive != search_case_sensitive ||
                completed.value.use_regex != search_use_regex) return;
            const auto& payload = completed.value;
            if (payload.csv_source != result_csv || payload.document_source != result_document) return;
            active_match = 0;
            search_issue_text = payload.error.empty()
                ? std::string{}
                : tr(i18n::Text::InvalidRegex);
            search_issue_detail_text = payload.error;
            search_matches = payload.document_matches;
            csv_search_matches = payload.csv_matches;
            if (!csv_search_matches.empty()) {
                const auto& first = csv_search_matches.front();
                result_scroll = csv_match_vertical_offset(first.row);
                csv_horizontal_scroll = static_cast<float>(first.column) * 180.0f;
            } else if (!search_matches.empty()) {
                reveal_result_line(search_matches.front().line);
            }
            core::platform::requestUiUpdate();
        });
}

void analyze_input(bool debounce = false) {
    const std::string snapshot = input_text;
    const int requested_mode = processing_mode_index;
    const int requested_override = input_type_override_index;
    const int requested_csv_delimiter = csv_delimiter_index;
    const bool requested_first_row_header = csv_first_row_header;
    status_text = debounce ? tr(i18n::Text::WaitingForInput) : tr(i18n::Text::Processing);
    issue_text.clear();
    issue_detail_text.clear();
    core::platform::requestUiUpdate();

    async::restart(
        "zeus.process.input",
        [snapshot, requested_mode, requested_override, requested_csv_delimiter,
         requested_first_row_header, debounce](const async::CancelToken& token) {
            if (debounce) {
                for (int elapsed = 0; elapsed < 180 && !token.canceled(); elapsed += 20) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
            }

            AnalysisPayload payload;
            payload.first_row_header = requested_first_row_header;
            if (token.canceled()) return payload;
            const auto started = std::chrono::steady_clock::now();
            const zeus::ProcessResult detected = zeus::process_text(snapshot, zeus::ProcessingMode::Auto);
            payload.detected = input_override_kind_for(requested_override, detected.detected);
            const zeus::ProcessingMode override_mode = input_override_mode_for(requested_override);
            const zeus::ProcessResult base_result = override_mode == zeus::ProcessingMode::Auto
                ? detected
                : zeus::process_text(snapshot, override_mode);
            const zeus::ProcessingMode action_mode = processing_mode_for(
                requested_mode, payload.detected);
            payload.result = action_mode == zeus::ProcessingMode::Auto
                ? base_result
                : zeus::process_text(snapshot, action_mode);
            if (token.canceled()) return payload;

            if (payload.result.tabular) {
                const std::string& table_source = payload.result.value.empty()
                    ? snapshot : payload.result.value;
                const char delimiter = requested_mode == 15
                    ? ',' : csv_delimiter_for(requested_csv_delimiter);
                const auto parsed = zeus::parse_csv(table_source, delimiter, true);
                if (parsed.ok) {
                    payload.csv = std::make_shared<zeus::CsvDocument>(parsed.document);
                } else {
                    payload.result.ok = false;
                    payload.result.label = "CSV";
                    payload.result.error_code = "CSV_PARSE_ERROR";
                    payload.result.error_message = parsed.error;
                }
            }
            const std::string display = payload.result.value.empty() ? snapshot : payload.result.value;
            if (payload.result.detected == zeus::ContentKind::Xml) {
                payload.document = std::make_shared<zeus::HighlightedDocument>(
                    zeus::HighlightedDocument::xml(display));
            } else if (payload.result.detected == zeus::ContentKind::Yaml) {
                payload.document = std::make_shared<zeus::HighlightedDocument>(
                    zeus::HighlightedDocument::yaml(display));
            } else {
                payload.document = std::make_shared<zeus::HighlightedDocument>(
                    payload.result.structured ? zeus::HighlightedDocument::json(display)
                                              : zeus::HighlightedDocument::plain(display));
            }
            payload.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - started).count();
            return payload;
        },
        [](const async::Result<AnalysisPayload>& completed) {
            if (!completed.ok || !completed.value.document) return;
            const AnalysisPayload& payload = completed.value;
            detected_input_kind = payload.detected;
            result_folds.clear();
            result_document = payload.document;
            result_csv = payload.csv;
            const auto& result = payload.result;
            if (!result.ok) {
                issue_text = compact_issue(
                    result.error_message.empty() ? result.error_code : result.error_message);
                issue_detail_text = result.error_message.empty()
                    ? result.error_code : result.error_message;
                if (!result.error_code.empty() && result.error_code != issue_detail_text) {
                    issue_detail_text += "\nCode: " + result.error_code;
                }
                if (result.error_line != 0) {
                    issue_text += " · L" + std::to_string(result.error_line) +
                                  ":C" + std::to_string(result.error_column);
                    issue_detail_text += "\nL" + std::to_string(result.error_line) +
                                         ":C" + std::to_string(result.error_column);
                }
                status_text = std::string(tr(i18n::Text::Invalid)) + " " + result.label;
            } else if (payload.csv) {
                issue_text.clear();
                issue_detail_text.clear();
                const std::size_t data_rows = payload.csv->rows.size() -
                    (payload.first_row_header && !payload.csv->rows.empty() ? 1 : 0);
                status_text = "CSV · " + std::to_string(data_rows) + " " +
                              tr(i18n::Text::Rows) + " · " +
                              std::to_string(payload.elapsed_ms) + " ms";
            } else {
                issue_text.clear();
                issue_detail_text.clear();
                status_text = result.label + " · " + std::to_string(result.value.size()) +
                              " " + tr(i18n::Text::Bytes) + " · " +
                              std::to_string(payload.elapsed_ms) + " ms";
            }
            result_scroll = 0.0f;
            csv_horizontal_scroll = 0.0f;
            selected_csv_row = no_csv_cell;
            selected_csv_column = no_csv_cell;
            result_selection.clear();
            update_search();
            request_full_repaint();
        });
}

void move_match(int direction) {
    const std::size_t match_count = result_csv ? csv_search_matches.size() : search_matches.size();
    if (match_count == 0) {
        return;
    }
    const std::int64_t count = static_cast<std::int64_t>(match_count);
    const std::int64_t current = static_cast<std::int64_t>(active_match);
    active_match = static_cast<std::size_t>((current + direction + count) % count);
    if (result_csv) {
        const auto& match = csv_search_matches[active_match];
        result_scroll = csv_match_vertical_offset(match.row);
        csv_horizontal_scroll = static_cast<float>(match.column) * 180.0f;
    } else {
        reveal_result_line(search_matches[active_match].line);
    }
    core::platform::requestUiUpdate();
}

void copy_result(bool selection_only) {
    if (result_csv) {
        const bool selected = selected_csv_row < result_csv->rows.size() &&
            selected_csv_column < result_csv->rows[selected_csv_row].size();
        if (selection_only && !selected) return;
        const std::string value = selected
            ? result_csv->rows[selected_csv_row][selected_csv_column]
            : result_csv->to_tsv();
        if (value.empty() && selection_only) return;
        core::window::setClipboardText(value);
        status_text = std::string(tr(i18n::Text::Copied)) + " " +
                      std::to_string(value.size()) + " " + tr(i18n::Text::Bytes);
        core::platform::requestUiUpdate();
        return;
    }
    if (!result_document) {
        return;
    }
    std::string value = result_selection.selected_text(*result_document);
    if (value.empty() && !selection_only) {
        value = result_document->text();
    }
    if (value.empty()) {
        return;
    }
    core::window::setClipboardText(value);
    status_text = std::string(tr(i18n::Text::Copied)) + " " +
                  std::to_string(value.size()) + " " + tr(i18n::Text::Bytes);
    core::platform::requestUiUpdate();
}

void compute_crypto_output(zeus::DigestAlgorithm algorithm) {
    const zeus::CryptoResult result = crypto_hmac
        ? zeus::compute_hmac(input_text, hmac_key, algorithm)
        : zeus::compute_digest(input_text, algorithm);
    const std::string name = crypto_hmac
        ? "HMAC-" + std::string(zeus::digest_algorithm_name(algorithm))
        : zeus::digest_algorithm_name(algorithm);
    if (!result.ok) {
        issue_text = result.error;
        status_text = name;
        return;
    }
    std::string display = name + "\n\nHex\n" + result.hex +
                          "\n\nBase64\n" + result.base64;
    if (zeus::digest_algorithm_is_weak(algorithm)) {
        display += "\n\n⚠ " + std::string(tr(i18n::Text::WeakAlgorithm));
    }
    result_folds.clear();
    result_document = std::make_shared<zeus::HighlightedDocument>(
        zeus::HighlightedDocument::plain(std::move(display)));
    result_csv.reset();
    result_scroll = 0.0f;
    result_selection.clear();
    issue_text.clear();
    status_text = name + " · Hex + Base64";
    core::platform::requestUiUpdate();
}

} // namespace

void compose(eui::Ui& ui, const eui::Screen& screen) {
    if (!result_document) {
        result_document = std::make_shared<zeus::HighlightedDocument>(
            zeus::HighlightedDocument::plain(tr(i18n::Text::Processing)));
        analyze_input(false);
    }

    const bool dark_theme = use_dark_theme();
    const auto tokens = theme_tokens(dark_theme);
    auto language_tokens = tokens;
    language_tokens.metrics.typography.body = 18.0f;
    language_tokens.metrics.typography.option = 18.0f;
    language_tokens.metrics.spacing.large = 23.0f;
    const float margin = 18.0f;
    const float header_height = 46.0f;
    const float actions_height = crypto_panel_open ? 88.0f : 44.0f;
    const float bottom_bar_height = 42.0f;
    const float content_width = std::max(480.0f, screen.width - margin * 2.0f);
    const float available_height = std::max(
        500.0f,
        screen.height - margin * 2.0f - header_height - actions_height - bottom_bar_height - 26.0f);
    const float input_height = std::max(190.0f, available_height * 0.40f);
    const float actions_y = margin + header_height + input_height + 8.0f;
    const float result_y = actions_y + actions_height + 8.0f;
    const float bottom_bar_y = screen.height - margin - bottom_bar_height;
    const float result_height = std::max(220.0f, bottom_bar_y - result_y - 8.0f);
    const float header_spacer_width = std::max(0.0f, content_width - 474.0f);
    const float theme_button_center_x = margin + 197.0f + header_spacer_width;

    ui.stack("root")
        .size(screen.width, screen.height)
        .content([&] {
            ui.rect("root.background")
                .size(screen.width, screen.height)
                .color(tokens.background)
                .dirtyKey("root.repaint." + std::to_string(full_repaint_revision))
                .build();

            ui.row("header")
                .position(margin, margin)
                .size(content_width, header_height)
                .zIndex(300)
                .gap(8.0f)
                .alignItems(eui::Align::CENTER)
                .content([&] {
                    ui.text("header.title")
                        .size(160.0f, header_height)
                        .text("Zeus Tools")
                        .fontFamily(fonts::ui())
                        .fontSize(22.0f)
                        .fontWeight(800)
                        .verticalAlign(eui::VerticalAlign::Center)
                        .color(tokens.text)
                        .build();

                    ui.rect("header.spacer")
                        .size(header_spacer_width, 1.0f)
                        .color({0.0f, 0.0f, 0.0f, 0.0f})
                        .build();

                    components::button(ui, "header.theme")
                        .size(42.0f, 38.0f)
                        .text("")
                        .icon(theme_icon())
                        .iconSize(18.0f)
                        .theme(tokens, false)
                        .onClick([] {
                            theme_preference_index = (theme_preference_index + 1) % 3;
                            zeus::save_theme_preference(
                                static_cast<zeus::ThemePreference>(theme_preference_index));
                            request_full_repaint();
                        })
                        .build();

                    ui.stack("header.language.slot")
                        .size(152.0f, 38.0f)
                        .zIndex(310)
                        .content([&] {
                            components::dropdown(ui, "header.language")
                                .size(152.0f, 38.0f)
                                .items(language_items())
                                .selected(locale_preference_index)
                                .open(language_dropdown_open)
                                .itemHeight(38.0f)
                                .zIndex(310)
                                .theme(language_tokens)
                                .transition(eui::Transition{})
                                .onOpenChange([](bool open) {
                                    language_dropdown_open = open;
                                    request_full_repaint();
                                })
                                .onChange([](int index) {
                                    language_dropdown_open = false;
                                    locale_preference_index = index;
                                    zeus::save_locale_preference(
                                        static_cast<zeus::LocalePreference>(index));
                                    analyze_input();
                                    core::platform::requestUiUpdate();
                                })
                                .build();
                        })
                        .build();

                    components::button(ui, "header.clear")
                        .size(88.0f, 38.0f)
                        .text(tr(i18n::Text::Clear))
                        .fontSize(20.0f)
                        .theme(tokens, false)
                        .onClick([] {
                            input_text.clear();
                            search_query.clear();
                            hmac_key.clear();
                            processing_mode_index = 0;
                            input_type_override_index = 0;
                            input_type_dropdown_open = false;
                            csv_delimiter_index = 0;
                            csv_delimiter_dropdown_open = false;
                            csv_first_row_header = true;
                            crypto_panel_open = false;
                            crypto_hmac = false;
                            analyze_input();
                        })
                        .build();

                })
                .build();

            components::tooltip(ui, "header.theme.tooltip")
                .source("header.theme.bg")
                .value(theme_tooltip())
                .anchor(theme_button_center_x, margin + header_height - 2.0f)
                .bounds(screen.width, screen.height)
                .theme(tokens)
                .zIndex(1000)
                .build();

            components::input(ui, "input.editor")
                .position(margin, margin + header_height)
                .size(content_width, input_height)
                .value(input_text)
                .placeholder(tr(i18n::Text::PasteOrTypeJson))
                .multiline(true)
                .fontFamily(fonts::code())
                .fontSize(14.5f)
                .theme(tokens)
                .onChange([](const std::string& value) {
                    input_text = value;
                    processing_mode_index = 0;
                    analyze_input(true);
                })
                .build();

            ui.rect("actions.background")
                .position(margin, actions_y)
                .size(content_width, actions_height)
                .color(dark_theme ? eui::Color{0.070f, 0.080f, 0.098f, 1.0f}
                                  : eui::Color{0.965f, 0.970f, 0.982f, 1.0f})
                .radius(7.0f)
                .border(1.0f, tokens.border)
                .build();

            constexpr float action_gap = 8.0f;
            ui.row("actions")
                .position(margin + 16.0f, actions_y + 7.0f)
                .size(content_width - 32.0f, 30.0f)
                .zIndex(200)
                .gap(action_gap)
                .alignItems(eui::Align::CENTER)
                .content([&] {
                    ui.stack("actions.input.type.slot")
                        .size(116.0f, 30.0f)
                        .zIndex(100)
                        .content([&] {
                            components::dropdown(ui, "actions.input.type")
                                .size(116.0f, 30.0f)
                                .items(input_type_items())
                                .selected(input_type_override_index)
                                .open(input_type_dropdown_open)
                                .itemHeight(30.0f)
                                .zIndex(100)
                                .theme(tokens)
                                .transition(eui::Transition{})
                                .onOpenChange([](bool open) {
                                    input_type_dropdown_open = open;
                                    request_full_repaint();
                                })
                                .onChange([](int index) {
                                    input_type_dropdown_open = false;
                                    input_type_override_index = index;
                                    processing_mode_index = 0;
                                    analyze_input();
                                })
                                .build();
                        })
                        .build();

                    const auto action = [&](const std::string& id,
                                            const std::string& label,
                                            int index,
                                            float width) {
                        const bool active = processing_mode_index == index;
                        auto button = components::button(ui, id)
                            .size(width, 30.0f)
                            .text(label)
                            .fontSize(20.0f)
                            .theme(tokens, active)
                            .radius(5.0f)
                            .onClick([index] {
                                if (index == 15) {
                                    csv_delimiter_index = 0;
                                    csv_first_row_header = true;
                                }
                                processing_mode_index = index;
                                analyze_input();
                            });
                        if (!active) {
                            button.colors(
                                      {0.0f, 0.0f, 0.0f, 0.0f},
                                      tokens.surfaceHover,
                                      tokens.surfaceActive)
                                  .textColor(tokens.text)
                                  .border(0.0f, {0.0f, 0.0f, 0.0f, 0.0f})
                                  .shadow(0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f});
                        }
                        button.build();
                    };
                    std::string detected_label = zeus::content_kind_name(detected_input_kind);
                    if (detected_input_kind == zeus::ContentKind::UrlEncoded) detected_label = "URL";
                    if (detected_input_kind == zeus::ContentKind::JsonEscaped) detected_label = "Esc JSON";
                    ui.text("actions.detected")
                        .size(64.0f, 30.0f)
                        .text(detected_label)
                        .fontFamily(fonts::ui())
                        .fontSize(18.0f)
                        .fontWeight(700)
                        .verticalAlign(eui::VerticalAlign::Center)
                        .color(tokens.text)
                        .build();

                    if (detected_input_kind == zeus::ContentKind::JsonEscaped) {
                        action("actions.json.unescape", tr(i18n::Text::Unescape), 0, 76.0f);
                    } else if (detected_input_kind == zeus::ContentKind::Json) {
                        action("actions.format", tr(i18n::Text::Format), 0, 64.0f);
                        action("actions.minify", tr(i18n::Text::Minify), 1, 62.0f);
                        action("actions.escape", tr(i18n::Text::Escape), 6, 62.0f);
                        action("actions.json.to_yaml", "→ YAML", 11, 68.0f);
                        action("actions.json.to_xml", "→ XML", 13, 62.0f);
                        action("actions.json.to_csv", "→ CSV", 15, 62.0f);
                    } else if (detected_input_kind == zeus::ContentKind::Xml) {
                        action("actions.xml.format", tr(i18n::Text::Format), 9, 64.0f);
                        action("actions.xml.to_json", "→ JSON", 16, 68.0f);
                    } else if (detected_input_kind == zeus::ContentKind::Yaml) {
                        action("actions.yaml.format", tr(i18n::Text::Format), 10, 64.0f);
                        action("actions.yaml.to_json", "→ JSON", 12, 68.0f);
                    } else if (detected_input_kind == zeus::ContentKind::Csv) {
                        action("actions.table", tr(i18n::Text::Table), 0, 60.0f);
                        ui.stack("actions.csv.delimiter.slot")
                            .size(118.0f, 30.0f)
                            .zIndex(110)
                            .content([&] {
                                components::dropdown(ui, "actions.csv.delimiter")
                                    .size(118.0f, 30.0f)
                                    .items(csv_delimiter_items())
                                    .selected(csv_delimiter_index)
                                    .open(csv_delimiter_dropdown_open)
                                    .itemHeight(30.0f)
                                    .zIndex(110)
                                    .theme(tokens)
                                    .transition(eui::Transition{})
                                    .onOpenChange([](bool open) {
                                        csv_delimiter_dropdown_open = open;
                                        request_full_repaint();
                                    })
                                    .onChange([](int index) {
                                        csv_delimiter_dropdown_open = false;
                                        csv_delimiter_index = index;
                                        processing_mode_index = 0;
                                        result_scroll = 0.0f;
                                        csv_horizontal_scroll = 0.0f;
                                        selected_csv_row = no_csv_cell;
                                        selected_csv_column = no_csv_cell;
                                        analyze_input();
                                    })
                                    .build();
                            })
                            .build();
                        components::button(ui, "actions.csv.header")
                            .size(106.0f, 30.0f)
                            .text(std::string(tr(i18n::Text::FirstRowHeader)) + ":" +
                                  (csv_first_row_header ? tr(i18n::Text::On) : tr(i18n::Text::Off)))
                            .fontSize(18.0f)
                            .theme(tokens, csv_first_row_header)
                            .radius(5.0f)
                            .onClick([] {
                                csv_first_row_header = !csv_first_row_header;
                                result_scroll = 0.0f;
                                selected_csv_row = no_csv_cell;
                                selected_csv_column = no_csv_cell;
                                analyze_input();
                            })
                            .build();
                    }
                    if (detected_input_kind == zeus::ContentKind::Base64) {
                        action("actions.base64.decode", tr(i18n::Text::Decode), 0, 64.0f);
                    } else if (detected_input_kind == zeus::ContentKind::UrlEncoded) {
                        action("actions.url.decode", tr(i18n::Text::Decode), 0, 64.0f);
                    } else if (detected_input_kind == zeus::ContentKind::Jwt) {
                        action("actions.jwt.inspect", tr(i18n::Text::Inspect), 0, 64.0f);
                    }

                    if (detected_input_kind != zeus::ContentKind::Empty) {
                        ui.rect("actions.common.divider")
                            .size(1.0f, 22.0f)
                            .color(tokens.border)
                            .build();
                        action("actions.base64.encode", tr(i18n::Text::Base64Encode), 2, 104.0f);
                        action("actions.url.encode", tr(i18n::Text::UrlEncode), 3, 98.0f);
                        action("actions.upper", tr(i18n::Text::Upper), 7, 56.0f);
                        action("actions.lower", tr(i18n::Text::Lower), 8, 56.0f);
                        components::button(ui, "actions.digest")
                            .size(90.0f, 30.0f)
                            .text(std::string(crypto_panel_open ? "▾ " : "▸ ") +
                                  tr(i18n::Text::Digest))
                            .fontSize(19.0f)
                            .theme(tokens, false)
                            .radius(5.0f)
                            .colors(
                                {0.0f, 0.0f, 0.0f, 0.0f},
                                tokens.surfaceHover,
                                tokens.surfaceActive)
                            .textColor(tokens.text)
                            .border(0.0f, {0.0f, 0.0f, 0.0f, 0.0f})
                            .shadow(0.0f, 0.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f})
                            .onClick([] {
                                crypto_panel_open = !crypto_panel_open;
                                if (!crypto_panel_open) {
                                    crypto_hmac = false;
                                    std::fill(hmac_key.begin(), hmac_key.end(), '\0');
                                    hmac_key.clear();
                                    hmac_key.shrink_to_fit();
                                }
                                core::platform::requestUiUpdate();
                            })
                            .build();
                    }
                })
                .build();

            if (crypto_panel_open) {
                ui.row("actions.crypto")
                    .position(margin + 16.0f, actions_y + 47.0f)
                    .size(content_width - 32.0f, 34.0f)
                    .gap(8.0f)
                    .alignItems(eui::Align::CENTER)
                    .content([&] {
                        const auto algorithm_button = [&](const std::string& id,
                                                          const std::string& label,
                                                          zeus::DigestAlgorithm algorithm,
                                                          float width) {
                            components::button(ui, id)
                                .size(width, 32.0f)
                                .text(label)
                                .fontSize(18.0f)
                                .theme(tokens, false)
                                .onClick([algorithm] { compute_crypto_output(algorithm); })
                                .build();
                        };

                        ui.text("actions.crypto.label")
                            .size(62.0f, 30.0f)
                            .text(crypto_hmac ? tr(i18n::Text::Hmac) : tr(i18n::Text::Digest))
                            .fontFamily(fonts::ui())
                            .fontSize(17.0f)
                            .fontWeight(700)
                            .verticalAlign(eui::VerticalAlign::Center)
                            .color(tokens.text)
                            .build();
                        algorithm_button("actions.crypto.md5", "MD5", zeus::DigestAlgorithm::Md5, 62.0f);
                        algorithm_button("actions.crypto.sha1", "SHA-1", zeus::DigestAlgorithm::Sha1, 72.0f);
                        algorithm_button("actions.crypto.sha256", "SHA-256", zeus::DigestAlgorithm::Sha256, 86.0f);
                        algorithm_button("actions.crypto.sha512", "SHA-512", zeus::DigestAlgorithm::Sha512, 86.0f);

                        components::button(ui, "actions.crypto.hmac")
                            .size(76.0f, 32.0f)
                            .text("HMAC")
                            .fontSize(18.0f)
                            .theme(tokens, crypto_hmac)
                            .onClick([] {
                                crypto_hmac = !crypto_hmac;
                                if (!crypto_hmac) {
                                    std::fill(hmac_key.begin(), hmac_key.end(), '\0');
                                    hmac_key.clear();
                                    hmac_key.shrink_to_fit();
                                }
                                core::platform::requestUiUpdate();
                            })
                            .build();

                        if (crypto_hmac) {
                            components::input(ui, "actions.crypto.key")
                                .size(std::max(150.0f, content_width - 520.0f), 30.0f)
                                .value(hmac_key)
                                .placeholder(tr(i18n::Text::HmacKey))
                                .fontFamily(fonts::code())
                                .fontSize(13.0f)
                                .theme(tokens)
                                .onChange([](const std::string& value) { hmac_key = value; })
                                .build();
                        }
                    })
                    .build();
            }

            ui.rect("result.background")
                .position(margin, result_y)
                .size(content_width, result_height)
                .color(dark_theme ? eui::Color{0.075f, 0.086f, 0.106f, 1.0f}
                                  : eui::Color{0.985f, 0.988f, 0.995f, 1.0f})
                .radius(8.0f)
                .border(1.0f, tokens.border)
                .build();

            const auto document = result_document;
            constexpr float row_height = 25.0f;
            if (result_csv && !result_csv->rows.empty()) {
                zeus::app_components::csvTableView(ui, "result.csv.table")
                    .position(margin + 8.0f, result_y + 6.0f)
                    .size(content_width - 16.0f, result_height - 12.0f)
                    .document(result_csv)
                    .scroll(result_scroll, csv_horizontal_scroll)
                    .selection(selected_csv_row, selected_csv_column)
                    .firstRowHeader(csv_first_row_header)
                    .theme(tokens)
                    .searchMatches(csv_search_matches, active_match)
                    .build();
            } else {
                zeus::app_components::richTextView(ui, "result.document")
                    .position(margin + 8.0f, result_y + 6.0f)
                    .size(content_width - 16.0f, result_height - 12.0f)
                    .document(document)
                    .selection(result_selection)
                    .folds(result_folds)
                    .scroll(result_scroll)
                    .rowHeight(row_height)
                    .dark(dark_theme)
                    .theme(tokens)
                    .searchMatches(search_matches, active_match)
                    .onCopy([] { copy_result(true); })
                    .build();
            }

            const std::string visible_issue = issue_text.empty() ? search_issue_text : issue_text;
            const std::string visible_issue_detail = issue_text.empty()
                ? search_issue_detail_text : issue_detail_text;
            const std::string footer = visible_issue.empty()
                ? status_text
                : status_text + " · " + visible_issue;
            const bool csv_cell_selected = result_csv && selected_csv_row < result_csv->rows.size() &&
                selected_csv_column < result_csv->rows[selected_csv_row].size();
            const std::size_t selected_bytes = csv_cell_selected
                ? result_csv->rows[selected_csv_row][selected_csv_column].size()
                : result_document ? result_selection.selected_length() : 0;
            const std::string selection_status = csv_cell_selected
                ? " · R" + std::to_string(selected_csv_row + 1) +
                      " C" + std::to_string(selected_csv_column + 1) + " · " +
                      tr(i18n::Text::Selected) + " " + std::to_string(selected_bytes) +
                      " " + tr(i18n::Text::Bytes)
                : selected_bytes > 0
                    ? " · " + std::string(tr(i18n::Text::Selected)) + " " +
                          std::to_string(selected_bytes) + " " + tr(i18n::Text::Bytes)
                    : std::string{};
            const std::size_t total_matches = result_csv
                ? csv_search_matches.size() : search_matches.size();
            const std::string count = total_matches == 0
                ? "0 / 0"
                : std::to_string(active_match + 1) + " / " +
                      std::to_string(total_matches);
            const float bottom_inner_width = content_width - 12.0f;
            const float search_width = std::clamp(content_width * 0.35f, 150.0f, 380.0f);
            const float status_width = std::max(52.0f, bottom_inner_width - search_width - 361.0f);

            ui.rect("bottom.background")
                .position(margin, bottom_bar_y)
                .size(content_width, bottom_bar_height)
                .color(dark_theme ? eui::Color{0.070f, 0.080f, 0.098f, 1.0f}
                                  : eui::Color{0.965f, 0.970f, 0.982f, 1.0f})
                .radius(7.0f)
                .border(1.0f, tokens.border)
                .build();

            ui.row("bottom.controls")
                .position(margin + 6.0f, bottom_bar_y + 4.0f)
                .size(bottom_inner_width, bottom_bar_height - 8.0f)
                .gap(5.0f)
                .alignItems(eui::Align::CENTER)
                .content([&] {
                    ui.text("bottom.status")
                        .size(status_width, 34.0f)
                        .text(footer + selection_status)
                        .fontFamily(fonts::ui())
                        .fontSize(14.0f)
                        .interactive(!visible_issue_detail.empty())
                        .cursor(core::CursorShape::Arrow)
                        .verticalAlign(eui::VerticalAlign::Center)
                        .color(visible_issue.empty()
                            ? (dark_theme ? eui::Color{0.55f, 0.68f, 0.82f, 1.0f}
                                          : eui::Color{0.18f, 0.40f, 0.66f, 1.0f})
                            : (dark_theme ? eui::Color{0.96f, 0.45f, 0.45f, 1.0f}
                                          : eui::Color{0.72f, 0.12f, 0.16f, 1.0f}))
                        .build();

                    components::input(ui, "bottom.search")
                        .size(search_width, 34.0f)
                        .value(search_query)
                        .placeholder(tr(i18n::Text::SearchResult))
                        .fontFamily(fonts::ui())
                        .theme(tokens)
                        .onChange([](const std::string& value) {
                            search_query = value;
                            update_search();
                        })
                        .build();

                    components::button(ui, "bottom.search.case")
                        .size(42.0f, 34.0f)
                        .text("Aa")
                        .fontSize(16.0f)
                        .theme(tokens, search_case_sensitive)
                        .onClick([] {
                            search_case_sensitive = !search_case_sensitive;
                            update_search();
                        })
                        .build();

                    components::button(ui, "bottom.search.regex")
                        .size(42.0f, 34.0f)
                        .text(".*")
                        .fontSize(16.0f)
                        .theme(tokens, search_use_regex)
                        .onClick([] {
                            search_use_regex = !search_use_regex;
                            update_search();
                        })
                        .build();

                    components::button(ui, "bottom.previous")
                        .size(34.0f, 34.0f)
                        .text("↑")
                        .fontSize(19.0f)
                        .theme(tokens, false)
                        .onClick([] { move_match(-1); })
                        .build();

                    components::button(ui, "bottom.next")
                        .size(34.0f, 34.0f)
                        .text("↓")
                        .fontSize(19.0f)
                        .theme(tokens, false)
                        .onClick([] { move_match(1); })
                        .build();

                    ui.text("bottom.match.count")
                        .size(58.0f, 34.0f)
                        .text(count)
                        .fontSize(14.0f)
                        .verticalAlign(eui::VerticalAlign::Center)
                        .color(dark_theme ? eui::Color{0.64f, 0.69f, 0.78f, 1.0f}
                                          : eui::Color{0.38f, 0.42f, 0.50f, 1.0f})
                        .build();

                    components::button(ui, "bottom.copy")
                        .size(116.0f, 34.0f)
                        .text((result_csv ? !csv_cell_selected : result_selection.empty())
                            ? tr(i18n::Text::CopyAll)
                            : tr(i18n::Text::CopySelected))
                        .fontSize(18.0f)
                        .theme(tokens, false)
                        .onClick([] { copy_result(false); })
                        .build();
                })
                .build();

            if (!visible_issue_detail.empty()) {
                const float detail_width = std::min(560.0f, content_width - 12.0f);
                constexpr float detail_height = 76.0f;
                ui.stack("bottom.issue.detail")
                    .position(margin + 6.0f, bottom_bar_y - detail_height - 6.0f)
                    .size(detail_width, detail_height)
                    .zIndex(1200)
                    .hoverOpacityFrom("bottom.status")
                    .content([&] {
                        ui.rect("bottom.issue.detail.bg")
                            .size(detail_width, detail_height)
                            .color(dark_theme
                                ? eui::Color{0.105f, 0.115f, 0.140f, 0.98f}
                                : eui::Color{1.0f, 1.0f, 1.0f, 0.98f})
                            .radius(8.0f)
                            .border(1.0f, tokens.border)
                            .shadow(12.0f, 0.0f, 4.0f, {0.0f, 0.0f, 0.0f, 0.20f})
                            .build();
                        ui.text("bottom.issue.detail.text")
                            .position(12.0f, 8.0f)
                            .size(detail_width - 24.0f, detail_height - 16.0f)
                            .text(visible_issue_detail)
                            .fontFamily(fonts::ui())
                            .fontSize(14.0f)
                            .lineHeight(19.0f)
                            .wrap(true)
                            .verticalAlign(eui::VerticalAlign::Center)
                            .color(tokens.text)
                            .build();
                    })
                    .build();
            }
        })
        .build();
}

} // namespace app
