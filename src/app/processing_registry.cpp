#include "processing_registry.h"

#include "zeus/developer_tools.h"

#include <algorithm>

namespace app::processing {

namespace {

using Action = ActionDefinition;
using Label = ActionLabel;
using Kind = zeus::ContentKind;
using Mode = zeus::ProcessingMode;

const std::vector<Action> kActions = {
    {"auto", Mode::Auto, Kind::JsonEscaped, Label::Unescape, 76.0f},
    {"auto", Mode::Auto, Kind::Json, Label::Format, 64.0f},
    {"json.minify", Mode::JsonMinify, Kind::Json, Label::Minify, 62.0f},
    {"json.escape", Mode::JsonEscape, Kind::Json, Label::Escape, 62.0f},
    {"json.to_yaml", Mode::JsonToYaml, Kind::Json, Label::ToYaml, 68.0f},
    {"json.to_xml", Mode::JsonToXml, Kind::Json, Label::ToXml, 62.0f},
    {"json.to_csv", Mode::JsonToCsv, Kind::Json, Label::ToCsv, 62.0f, false, false, true},
    {"json.to_toml", Mode::JsonToToml, Kind::Json, Label::ToToml, 72.0f},
    {"auto", Mode::Auto, Kind::Xml, Label::Format, 64.0f},
    {"xml.to_json", Mode::XmlToJson, Kind::Xml, Label::ToJson, 68.0f},
    {"auto", Mode::Auto, Kind::Yaml, Label::Format, 64.0f},
    {"yaml.to_json", Mode::YamlToJson, Kind::Yaml, Label::ToJson, 68.0f},
    {"auto", Mode::Auto, Kind::Toml, Label::Format, 64.0f},
    {"toml.to_json", Mode::TomlToJson, Kind::Toml, Label::ToJson, 68.0f},
    {"auto", Mode::Auto, Kind::Ini, Label::Format, 64.0f},
    {"ini.to_json", Mode::IniToJson, Kind::Ini, Label::ToJson, 68.0f},
    {"auto", Mode::Auto, Kind::Csv, Label::Table, 60.0f},
    {"auto", Mode::Auto, Kind::Base64, Label::Decode, 64.0f},
    {"auto", Mode::Auto, Kind::UrlEncoded, Label::Decode, 64.0f},
    {"auto", Mode::Auto, Kind::Jwt, Label::Inspect, 64.0f},
    {"html.decode", Mode::HtmlEntityDecode, Kind::HtmlEntity, Label::Decode, 64.0f},
    {"hex.decode", Mode::HexDecode, Kind::HexEncoded, Label::Decode, 64.0f},
    {"timestamp.inspect", Mode::Timestamp, Kind::Text, Label::UnixTime, 56.0f, false, true},
    {"base64.encode", Mode::Base64Encode, Kind::Text, Label::Base64Encode, 104.0f, true},
    {"url.encode", Mode::UrlEncode, Kind::Text, Label::UrlEncode, 98.0f, true},
    {"html.encode", Mode::HtmlEntityEncode, Kind::Text, Label::HtmlEncode, 94.0f, true},
    {"hex.encode", Mode::HexEncode, Kind::Text, Label::HexEncode, 82.0f, true},
    {"text.upper", Mode::Upper, Kind::Text, Label::Upper, 56.0f, true},
    {"text.lower", Mode::Lower, Kind::Text, Label::Lower, 56.0f, true},
};

const std::vector<InputTypeDefinition> kInputTypes = {
    {"auto", "Auto", Mode::Auto, Kind::Text},
    {"json", "JSON", Mode::Json, Kind::Json},
    {"xml", "XML", Mode::Xml, Kind::Xml},
    {"yaml", "YAML", Mode::Yaml, Kind::Yaml},
    {"toml", "TOML", Mode::Toml, Kind::Toml},
    {"ini", "INI", Mode::Ini, Kind::Ini},
    {"csv", "CSV", Mode::Csv, Kind::Csv},
    {"base64", "Base64", Mode::Base64, Kind::Base64},
    {"url", "URL", Mode::UrlDecode, Kind::UrlEncoded},
    {"text", "Text", Mode::Text, Kind::Text},
};

} // namespace

bool action_applies(const Action& action, Kind kind, std::string_view input) {
    if (action.common) return kind != Kind::Empty;
    if (action.timestamp_candidate) return zeus::looks_like_unix_timestamp(std::string(input));
    return action.input_kind == kind;
}

const std::vector<ActionDefinition>& registered_actions() { return kActions; }
const std::vector<InputTypeDefinition>& registered_input_types() { return kInputTypes; }

const ActionDefinition* find_action(std::string_view id, Kind kind, std::string_view input) {
    const auto found = std::find_if(kActions.begin(), kActions.end(), [&](const Action& action) {
        return action.id == id && action_applies(action, kind, input);
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
