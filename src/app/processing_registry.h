#pragma once

#include "zeus/text_processor.h"

#include <string>
#include <string_view>
#include <vector>

namespace app::processing {

enum class ActionLabel {
    Format,
    Minify,
    Escape,
    Unescape,
    ToYaml,
    ToXml,
    ToCsv,
    ToToml,
    ToJson,
    Table,
    Decode,
    Inspect,
    UnixTime,
    Base64Encode,
    UrlEncode,
    HtmlEncode,
    HexEncode,
    Upper,
    Lower,
};

struct ActionDefinition {
    std::string_view id;
    zeus::ProcessingMode mode = zeus::ProcessingMode::Auto;
    zeus::ContentKind input_kind = zeus::ContentKind::Text;
    ActionLabel label = ActionLabel::Format;
    float width = 64.0f;
    bool common = false;
    bool timestamp_candidate = false;
    bool reset_csv_options = false;
};

struct InputTypeDefinition {
    std::string_view id;
    std::string_view label;
    zeus::ProcessingMode mode = zeus::ProcessingMode::Auto;
    zeus::ContentKind kind = zeus::ContentKind::Text;
};

const std::vector<ActionDefinition>& registered_actions();
const std::vector<InputTypeDefinition>& registered_input_types();

bool action_applies(
    const ActionDefinition& action,
    zeus::ContentKind kind,
    std::string_view input);
const ActionDefinition* find_action(
    std::string_view id,
    zeus::ContentKind kind,
    std::string_view input);
const InputTypeDefinition& find_input_type(std::string_view id);
const InputTypeDefinition& input_type_at(int index);
int input_type_index(std::string_view id);

} // namespace app::processing
