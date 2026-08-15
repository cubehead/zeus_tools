#include "processing_registry.h"

#include "zeus/developer_tools.h"

#include <algorithm>
#include <array>

namespace app::processing {

namespace {

using Action = ActionDefinition;
using Label = ActionLabel;
using Kind = zeus::ContentKind;
using Mode = zeus::ProcessingMode;

const std::vector<Action> kActions = {
    {Mode::Auto, Kind::JsonEscaped, Label::Unescape, 76.0f},
    {Mode::Auto, Kind::Json, Label::Format, 64.0f},
    {Mode::JsonMinify, Kind::Json, Label::Minify, 62.0f},
    {Mode::JsonEscape, Kind::Json, Label::Escape, 62.0f},
    {Mode::JsonToYaml, Kind::Json, Label::ToYaml, 68.0f},
    {Mode::JsonToXml, Kind::Json, Label::ToXml, 62.0f},
    {Mode::JsonToCsv, Kind::Json, Label::ToCsv, 62.0f, false, false, true},
    {Mode::JsonToToml, Kind::Json, Label::ToToml, 72.0f},
    {Mode::Auto, Kind::Xml, Label::Format, 64.0f},
    {Mode::XmlToJson, Kind::Xml, Label::ToJson, 68.0f},
    {Mode::Auto, Kind::Yaml, Label::Format, 64.0f},
    {Mode::YamlToJson, Kind::Yaml, Label::ToJson, 68.0f},
    {Mode::Auto, Kind::Toml, Label::Format, 64.0f},
    {Mode::TomlToJson, Kind::Toml, Label::ToJson, 68.0f},
    {Mode::Auto, Kind::Ini, Label::Format, 64.0f},
    {Mode::IniToJson, Kind::Ini, Label::ToJson, 68.0f},
    {Mode::Auto, Kind::Csv, Label::Table, 60.0f},
    {Mode::Auto, Kind::Base64, Label::Decode, 64.0f},
    {Mode::Auto, Kind::UrlEncoded, Label::Decode, 64.0f},
    {Mode::Auto, Kind::Jwt, Label::Inspect, 64.0f},
    {Mode::HtmlEntityDecode, Kind::HtmlEntity, Label::Decode, 64.0f},
    {Mode::HexDecode, Kind::HexEncoded, Label::Decode, 64.0f},
    {Mode::Timestamp, Kind::Text, Label::UnixTime, 56.0f, false, true},
    {Mode::Base64Encode, Kind::Text, Label::Base64Encode, 104.0f, true},
    {Mode::UrlEncode, Kind::Text, Label::UrlEncode, 98.0f, true},
    {Mode::HtmlEntityEncode, Kind::Text, Label::HtmlEncode, 94.0f, true},
    {Mode::HexEncode, Kind::Text, Label::HexEncode, 82.0f, true},
    {Mode::Upper, Kind::Text, Label::Upper, 56.0f, true},
    {Mode::Lower, Kind::Text, Label::Lower, 56.0f, true},
};

const std::vector<InputTypeDefinition> kInputTypes = {
    {"auto", Mode::Auto, Kind::Text},
    {"json", Mode::Json, Kind::Json},
    {"xml", Mode::Xml, Kind::Xml},
    {"yaml", Mode::Yaml, Kind::Yaml},
    {"toml", Mode::Toml, Kind::Toml},
    {"ini", Mode::Ini, Kind::Ini},
    {"csv", Mode::Csv, Kind::Csv},
    {"base64", Mode::Base64, Kind::Base64},
    {"url", Mode::UrlDecode, Kind::UrlEncoded},
    {"text", Mode::Text, Kind::Text},
};

using Syntax = DocumentSyntax;
constexpr std::array<ContentDefinition, static_cast<std::size_t>(Kind::Count)>
    kContentDefinitions{{
        {Kind::Empty, "Empty", "Empty", ".txt", Syntax::Plain},
        {Kind::Json, "JSON", "JSON", ".json", Syntax::Json},
        {Kind::Xml, "XML", "XML", ".xml", Syntax::Xml},
        {Kind::Yaml, "YAML", "YAML", ".yaml", Syntax::Yaml},
        {Kind::Toml, "TOML", "TOML", ".toml", Syntax::Toml},
        {Kind::Ini, "INI", "INI", ".ini", Syntax::Toml},
        {Kind::Jwt, "JWT", "JWT", ".txt", Syntax::Plain},
        {Kind::JsonEscaped, "JSON String", "Esc JSON", ".txt", Syntax::Plain},
        {Kind::Base64, "Base64", "Base64", ".txt", Syntax::Plain},
        {Kind::UrlEncoded, "URL", "URL", ".txt", Syntax::Plain},
        {Kind::HtmlEntity, "HTML Entity", "HTML Ent", ".txt", Syntax::Plain},
        {Kind::HexEncoded, "Hex", "Hex", ".txt", Syntax::Plain},
        {Kind::Csv, "CSV", "CSV", ".tsv", Syntax::Plain},
        {Kind::Text, "Text", "Text", ".txt", Syntax::Plain},
    }};

constexpr bool content_definitions_are_complete() {
    for (std::size_t index = 0; index < kContentDefinitions.size(); ++index) {
        if (static_cast<std::size_t>(kContentDefinitions[index].kind) != index) return false;
    }
    return true;
}

static_assert(content_definitions_are_complete(),
              "Every content kind must have one ordered content definition");

} // namespace

bool action_applies(const Action& action, Kind kind, std::string_view input) {
    if (action.common) return kind != Kind::Empty;
    if (action.timestamp_candidate) return zeus::looks_like_unix_timestamp(std::string(input));
    return action.input_kind == kind;
}

const std::vector<ActionDefinition>& registered_actions() { return kActions; }
const std::vector<InputTypeDefinition>& registered_input_types() { return kInputTypes; }

std::string_view action_id(const ActionDefinition& action) {
    return zeus::processing_mode_id(action.mode);
}

const ContentDefinition& content_definition(Kind kind) {
    const auto index = static_cast<std::size_t>(kind);
    return index < kContentDefinitions.size()
        ? kContentDefinitions[index]
        : kContentDefinitions[static_cast<std::size_t>(Kind::Text)];
}

const ActionDefinition* find_action(std::string_view id, Kind kind, std::string_view input) {
    const auto found = std::find_if(kActions.begin(), kActions.end(), [&](const Action& action) {
        return action_id(action) == id && action_applies(action, kind, input);
    });
    return found == kActions.end() ? nullptr : &*found;
}

const InputTypeDefinition& find_input_type(std::string_view id) {
    const auto found = std::find_if(kInputTypes.begin(), kInputTypes.end(),
        [&](const InputTypeDefinition& type) { return type.id == id; });
    return found == kInputTypes.end() ? kInputTypes.front() : *found;
}

const InputTypeDefinition& input_type_at(int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= kInputTypes.size()) {
        return kInputTypes.front();
    }
    return kInputTypes[static_cast<std::size_t>(index)];
}

int input_type_index(std::string_view id) {
    const auto found = std::find_if(kInputTypes.begin(), kInputTypes.end(),
        [&](const InputTypeDefinition& type) { return type.id == id; });
    return found == kInputTypes.end()
        ? 0 : static_cast<int>(std::distance(kInputTypes.begin(), found));
}

} // namespace app::processing
