#pragma once

#include "zeus/csv_document.h"
#include "zeus/locale_preference.h"
#include "zeus/text_document.h"
#include "zeus/text_processor.h"
#include "zeus/text_selection.h"
#include "zeus/theme_preference.h"

#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace app {

inline constexpr std::size_t kNoCsvCell = std::numeric_limits<std::size_t>::max();

struct SearchState {
    std::string query;
    std::string issue;
    std::string issue_detail;
    bool case_sensitive = false;
    bool use_regex = false;
    std::size_t active_match = 0;
    std::vector<zeus::SearchMatch> document_matches;
    std::vector<zeus::CsvSearchMatch> csv_matches;
};

struct ResultState {
    std::string status;
    std::string issue;
    std::string issue_detail;
    float scroll = 0.0f;
    float csv_horizontal_scroll = 0.0f;
    std::size_t selected_csv_row = kNoCsvCell;
    std::size_t selected_csv_column = kNoCsvCell;
    std::shared_ptr<zeus::HighlightedDocument> document;
    std::shared_ptr<zeus::CsvDocument> csv;
    zeus::TextSelection selection;
    zeus::TextFoldState folds;
    std::string decode_chain;
    bool can_continue_decode = false;
    zeus::ContentKind detected_input_kind = zeus::ContentKind::Text;
    zeus::ContentKind output_kind = zeus::ContentKind::Text;
};

struct CsvState {
    int delimiter_index = 0;
    bool delimiter_dropdown_open = false;
    bool first_row_header = true;
};

struct CryptoState {
    bool panel_open = false;
    bool hmac = false;
    std::string hmac_key;
    int message_source_index = 0;
    bool message_dropdown_open = false;
    int key_encoding_index = 0;
    bool key_encoding_dropdown_open = false;
    bool key_visible = false;
};

struct LayoutState {
    float input_ratio = 0.40f;
    float input_ratio_at_drag_start = 0.40f;
    bool splitter_hovered = false;
};

struct AppState {
    std::string input_text;
    SearchState search;
    ResultState result;
    CsvState csv;
    CryptoState crypto;
    LayoutState layout;
    std::string processing_action_id = "auto";
    std::string input_type_id = "auto";
    bool input_type_dropdown_open = false;
    bool language_dropdown_open = false;
    bool about_dialog_open = false;
    std::size_t full_repaint_revision = 0;
    int theme_preference_index = static_cast<int>(zeus::load_theme_preference());
    int locale_preference_index = static_cast<int>(zeus::load_locale_preference());
};

} // namespace app
