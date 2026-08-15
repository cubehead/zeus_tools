#pragma once

#include "app_state.h"
#include "i18n.h"

#include "components/theme.h"
#include "eui/dsl.h"

#include "zeus/crypto_service.h"

#include <string>
#include <vector>

namespace app::controller {

AppState& state();
void initialize_documentation_scenario();

const char* tr(i18n::Text text);
const char* theme_label();
unsigned int theme_icon();
std::string theme_tooltip();
bool use_dark_theme();
components::theme::ThemeColorTokens theme_tokens(bool dark);

std::vector<std::string> language_items();
std::vector<std::string> input_type_items();
std::vector<std::string> csv_delimiter_items();
std::vector<std::string> crypto_message_items();
std::vector<std::string> hmac_key_encoding_items();

void request_full_repaint();
void clear_hmac_key();
void clear_hmac_input_state(eui::Ui& ui);
void update_search();
void analyze_input(bool debounce = false);
bool oversized_input_paused();
bool oversized_input();
void process_oversized_input();
void continue_decode_one_layer();
void move_match(int direction);
void copy_result(bool selection_only);
void open_input_file();
void load_input_file(const std::string& path);
void export_result();
void compute_crypto_output(zeus::DigestAlgorithm algorithm);

} // namespace app::controller
