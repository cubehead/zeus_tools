#include "zeus/structured_converter.h"

#include "zeus/yaml_formatter.h"
#include "zeus/xml_formatter.h"

#include <pugixml.hpp>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <vector>
#include <string>

namespace zeus {
namespace {

ParseIssue conversion_issue(const char* code, const std::string& message) {
    ParseIssue issue;
    issue.code = code;
    issue.message = message;
    return issue;
}

void append_json_string(std::string& output, const std::string& value) {
    constexpr char hex[] = "0123456789ABCDEF";
    output.push_back('"');
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"': output += "\\\""; break;
        case '\\': output += "\\\\"; break;
        case '\b': output += "\\b"; break;
        case '\f': output += "\\f"; break;
        case '\n': output += "\\n"; break;
        case '\r': output += "\\r"; break;
        case '\t': output += "\\t"; break;
        default:
            if (ch < 0x20U) {
                output += "\\u00";
                output.push_back(hex[(ch >> 4U) & 0x0FU]);
                output.push_back(hex[ch & 0x0FU]);
            } else {
                output.push_back(static_cast<char>(ch));
            }
        }
    }
    output.push_back('"');
}

bool is_json_number(const std::string& value) {
    std::size_t i = 0;
    if (i < value.size() && value[i] == '-') ++i;
    if (i == value.size()) return false;
    if (value[i] == '0') {
        ++i;
        if (i < value.size() && std::isdigit(static_cast<unsigned char>(value[i]))) return false;
    } else if (value[i] >= '1' && value[i] <= '9') {
        while (i < value.size() && std::isdigit(static_cast<unsigned char>(value[i]))) ++i;
    } else {
        return false;
    }
    if (i < value.size() && value[i] == '.') {
        ++i;
        const std::size_t start = i;
        while (i < value.size() && std::isdigit(static_cast<unsigned char>(value[i]))) ++i;
        if (i == start) return false;
    }
    if (i < value.size() && (value[i] == 'e' || value[i] == 'E')) {
        ++i;
        if (i < value.size() && (value[i] == '+' || value[i] == '-')) ++i;
        const std::size_t start = i;
        while (i < value.size() && std::isdigit(static_cast<unsigned char>(value[i]))) ++i;
        if (i == start) return false;
    }
    return i == value.size();
}

std::string lower_ascii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool yaml_string_needs_quotes(const std::string& value) {
    if (value.empty() || is_json_number(value) ||
        std::isspace(static_cast<unsigned char>(value.front())) ||
        std::isspace(static_cast<unsigned char>(value.back()))) return true;
    const std::string lowered = lower_ascii(value);
    if (lowered == "true" || lowered == "false" || lowered == "null" || value == "~") return true;
    if (std::string("-?:,[]{}#&*!|>'\"%@`").find(value.front()) != std::string::npos) return true;
    return value.find(": ") != std::string::npos || value.find(" #") != std::string::npos ||
           value.find('\n') != std::string::npos || value.find('\r') != std::string::npos;
}

void emit_json_node_as_yaml(const YAML::Node& node, YAML::Emitter& emitter) {
    if (!node || node.IsNull()) {
        emitter << YAML::Null;
    } else if (node.IsSequence()) {
        emitter << YAML::BeginSeq;
        for (const auto& child : node) emit_json_node_as_yaml(child, emitter);
        emitter << YAML::EndSeq;
    } else if (node.IsMap()) {
        emitter << YAML::BeginMap;
        for (const auto& entry : node) {
            emitter << YAML::Key << entry.first.Scalar() << YAML::Value;
            emit_json_node_as_yaml(entry.second, emitter);
        }
        emitter << YAML::EndMap;
    } else if (node.Tag() == "!" || node.Tag() == "tag:yaml.org,2002:str") {
        const std::string value = node.Scalar();
        if (yaml_string_needs_quotes(value)) emitter << YAML::DoubleQuoted;
        emitter << value;
    } else {
        emitter << node;
    }
}

void append_xml_escaped(std::string& output, const std::string& value, bool attribute) {
    for (const unsigned char ch : value) {
        switch (ch) {
        case '&': output += "&amp;"; break;
        case '<': output += "&lt;"; break;
        case '>': output += "&gt;"; break;
        case '"': output += attribute ? "&quot;" : "\""; break;
        case '\'': output += attribute ? "&apos;" : "'"; break;
        default: output.push_back(static_cast<char>(ch)); break;
        }
    }
}

bool is_xml_element_name(const std::string& value) {
    if (value.empty()) return false;
    const auto first = static_cast<unsigned char>(value.front());
    if (!(std::isalpha(first) || first == '_')) return false;
    for (const unsigned char ch : value) {
        if (!(std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.')) return false;
    }
    const std::string lowered = lower_ascii(value);
    return lowered.rfind("xml", 0) != 0;
}

void append_json_xml_value(const YAML::Node& node, const std::string& name,
                           const std::string* original_key, std::string& output) {
    output.push_back('<');
    output += name;
    if (original_key != nullptr) {
        output += " key=\"";
        append_xml_escaped(output, *original_key, true);
        output.push_back('"');
    }
    if (!node || node.IsNull()) {
        output += " />";
        return;
    }
    output.push_back('>');
    if (node.IsScalar()) {
        append_xml_escaped(output, node.Scalar(), false);
    } else if (node.IsSequence()) {
        for (const auto& child : node) append_json_xml_value(child, "item", nullptr, output);
    } else if (node.IsMap()) {
        for (const auto& entry : node) {
            const std::string key = entry.first.Scalar();
            if (is_xml_element_name(key)) {
                append_json_xml_value(entry.second, key, nullptr, output);
            } else {
                append_json_xml_value(entry.second, "entry", &key, output);
            }
        }
    }
    output += "</";
    output += name;
    output.push_back('>');
}

bool scalar_to_json(const YAML::Node& node, std::string& output) {
    const std::string value = node.Scalar();
    const std::string tag = node.Tag();
    const bool forced_string = tag == "!" || tag == "tag:yaml.org,2002:str";
    if (!forced_string) {
        const std::string lowered = lower_ascii(value);
        if (lowered == "null" || value == "~") {
            output += "null";
            return true;
        }
        if (lowered == "true" || lowered == "false") {
            output += lowered;
            return true;
        }
        if (is_json_number(value)) {
            output += value;
            return true;
        }
    }
    append_json_string(output, value);
    return true;
}

void indent(std::string& output, int depth, int width) {
    output.append(static_cast<std::size_t>(depth * width), ' ');
}

bool has_non_whitespace(const std::string& value) {
    return std::any_of(value.begin(), value.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
}

struct XmlChildGroup {
    std::string name;
    std::vector<pugi::xml_node> nodes;
};

void xml_element_to_json(const pugi::xml_node& element, std::string& output,
                         int depth, int width) {
    std::vector<pugi::xml_node> element_children;
    std::string text;
    for (const auto& child : element.children()) {
        if (child.type() == pugi::node_element) {
            element_children.push_back(child);
        } else if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata) {
            text += child.value();
        }
    }

    const bool meaningful_text = has_non_whitespace(text);
    const bool has_attributes = element.first_attribute();
    const bool has_elements = !element_children.empty();
    if (!has_attributes && !has_elements) {
        append_json_string(output, text);
        return;
    }

    std::vector<XmlChildGroup> groups;
    if (!meaningful_text) {
        for (const auto& child : element_children) {
            const std::string name = child.name();
            const auto found = std::find_if(groups.begin(), groups.end(), [&](const XmlChildGroup& group) {
                return group.name == name;
            });
            if (found == groups.end()) groups.push_back({name, {child}});
            else found->nodes.push_back(child);
        }
    }

    std::size_t entry_count = 0;
    for (const auto& ignored : element.attributes()) {
        (void)ignored;
        ++entry_count;
    }
    entry_count += meaningful_text && has_elements ? 1U : groups.size();
    if (meaningful_text && !has_elements) ++entry_count;

    output.push_back('{');
    if (entry_count != 0) output.push_back('\n');
    std::size_t entry_index = 0;
    const auto begin_entry = [&](const std::string& key) {
        indent(output, depth + 1, width);
        append_json_string(output, key);
        output += ": ";
    };
    const auto end_entry = [&] {
        if (++entry_index < entry_count) output.push_back(',');
        output.push_back('\n');
    };

    for (const auto& attribute : element.attributes()) {
        begin_entry("@" + std::string(attribute.name()));
        append_json_string(output, attribute.value());
        end_entry();
    }

    if (meaningful_text && has_elements) {
        begin_entry("#content");
        output.push_back('[');
        std::vector<pugi::xml_node> content_nodes;
        for (const auto& child : element.children()) {
            if (child.type() == pugi::node_element || child.type() == pugi::node_pcdata ||
                child.type() == pugi::node_cdata) {
                content_nodes.push_back(child);
            }
        }
        if (!content_nodes.empty()) output.push_back('\n');
        for (std::size_t i = 0; i < content_nodes.size(); ++i) {
            const auto& child = content_nodes[i];
            indent(output, depth + 2, width);
            if (child.type() == pugi::node_element) {
                output += "{\n";
                indent(output, depth + 3, width);
                append_json_string(output, child.name());
                output += ": ";
                xml_element_to_json(child, output, depth + 3, width);
                output.push_back('\n');
                indent(output, depth + 2, width);
                output.push_back('}');
            } else {
                append_json_string(output, child.value());
            }
            if (i + 1 < content_nodes.size()) output.push_back(',');
            output.push_back('\n');
        }
        if (!content_nodes.empty()) indent(output, depth + 1, width);
        output.push_back(']');
        end_entry();
    } else {
        if (meaningful_text) {
            begin_entry("#text");
            append_json_string(output, text);
            end_entry();
        }
        for (const auto& group : groups) {
            begin_entry(group.name);
            if (group.nodes.size() == 1) {
                xml_element_to_json(group.nodes.front(), output, depth + 1, width);
            } else {
                output.push_back('[');
                output.push_back('\n');
                for (std::size_t i = 0; i < group.nodes.size(); ++i) {
                    indent(output, depth + 2, width);
                    xml_element_to_json(group.nodes[i], output, depth + 2, width);
                    if (i + 1 < group.nodes.size()) output.push_back(',');
                    output.push_back('\n');
                }
                indent(output, depth + 1, width);
                output.push_back(']');
            }
            end_entry();
        }
    }

    if (entry_count != 0) indent(output, depth, width);
    output.push_back('}');
}

bool node_to_json(const YAML::Node& node, std::string& output, int depth, int width,
                  ParseIssue& issue) {
    if (!node || node.IsNull()) {
        output += "null";
        return true;
    }
    if (node.IsScalar()) return scalar_to_json(node, output);
    if (node.IsSequence()) {
        output.push_back('[');
        if (node.size() != 0) output.push_back('\n');
        for (std::size_t i = 0; i < node.size(); ++i) {
            indent(output, depth + 1, width);
            if (!node_to_json(node[i], output, depth + 1, width, issue)) return false;
            if (i + 1 < node.size()) output.push_back(',');
            output.push_back('\n');
        }
        if (node.size() != 0) indent(output, depth, width);
        output.push_back(']');
        return true;
    }
    if (node.IsMap()) {
        output.push_back('{');
        if (node.size() != 0) output.push_back('\n');
        std::size_t index = 0;
        for (const auto& entry : node) {
            if (!entry.first.IsScalar()) {
                issue = conversion_issue("YAML_NON_SCALAR_KEY",
                    "YAML mappings with collection keys cannot be represented as JSON");
                return false;
            }
            indent(output, depth + 1, width);
            append_json_string(output, entry.first.Scalar());
            output += ": ";
            if (!node_to_json(entry.second, output, depth + 1, width, issue)) return false;
            if (++index < node.size()) output.push_back(',');
            output.push_back('\n');
        }
        if (node.size() != 0) indent(output, depth, width);
        output.push_back('}');
        return true;
    }
    issue = conversion_issue("YAML_UNSUPPORTED_NODE", "Unsupported YAML node type");
    return false;
}

std::string compact_json(std::string value) {
    std::string output;
    output.reserve(value.size());
    bool in_string = false;
    bool escaped = false;
    for (char ch : value) {
        if (in_string) {
            output.push_back(ch);
            if (escaped) escaped = false;
            else if (ch == '\\') escaped = true;
            else if (ch == '"') in_string = false;
        } else if (ch == '"') {
            in_string = true;
            output.push_back(ch);
        } else if (!std::isspace(static_cast<unsigned char>(ch))) {
            output.push_back(ch);
        }
    }
    return output;
}

std::string json_node_cell(const YAML::Node& node) {
    if (!node || node.IsNull()) return {};
    if (node.IsScalar()) return node.Scalar();
    std::string json;
    ParseIssue ignored;
    if (!node_to_json(node, json, 0, 2, ignored)) return {};
    return compact_json(std::move(json));
}

void append_csv_cell(std::string& output, const std::string& value) {
    const bool quote = value.find_first_of(",\"\r\n") != std::string::npos;
    if (!quote) {
        output += value;
        return;
    }
    output.push_back('"');
    for (char ch : value) {
        if (ch == '"') output += "\"\"";
        else output.push_back(ch);
    }
    output.push_back('"');
}

void append_csv_row(std::string& output, const std::vector<std::string>& row) {
    for (std::size_t i = 0; i < row.size(); ++i) {
        if (i != 0) output.push_back(',');
        append_csv_cell(output, row[i]);
    }
    output.push_back('\n');
}

} // namespace

FormatResult json_to_yaml(const std::string& input, int indent_width) {
    const auto json = format_json(input, 2);
    if (!json.ok) return json;
    try {
        const YAML::Node root = YAML::Load(json.value);
        YAML::Emitter emitter;
        emitter.SetIndent(static_cast<std::size_t>(std::max(1, indent_width)));
        emit_json_node_as_yaml(root, emitter);
        if (!emitter.good()) {
            return {false, {}, conversion_issue("JSON_TO_YAML_ERROR", emitter.GetLastError())};
        }
        return {true, emitter.c_str(), {}};
    } catch (const YAML::Exception& exception) {
        return {false, {}, conversion_issue("JSON_TO_YAML_ERROR", exception.msg)};
    }
}

FormatResult json_to_xml(const std::string& input, int indent_width) {
    const auto json = format_json(input, 2);
    if (!json.ok) return json;
    try {
        const YAML::Node root = YAML::Load(json.value);
        std::string compact;
        append_json_xml_value(root, "root", nullptr, compact);
        const auto xml = format_xml(compact, indent_width);
        if (!xml.ok) {
            return {false, {}, conversion_issue("JSON_TO_XML_ERROR", xml.issue.message)};
        }
        return xml;
    } catch (const YAML::Exception& exception) {
        return {false, {}, conversion_issue("JSON_TO_XML_ERROR", exception.msg)};
    }
}

FormatResult xml_to_json(const std::string& input, int indent_width) {
    const auto validated = format_xml(input, 2);
    if (!validated.ok) return validated;

    pugi::xml_document document;
    const unsigned int flags = pugi::parse_default | pugi::parse_comments |
                               pugi::parse_pi | pugi::parse_declaration;
    const auto parsed = document.load_buffer(
        input.data(), input.size(), flags, pugi::encoding_utf8);
    if (!parsed) {
        return {false, {}, conversion_issue("XML_TO_JSON_ERROR", parsed.description())};
    }

    const pugi::xml_node root = document.document_element();
    if (!root) {
        return {false, {}, conversion_issue("XML_TO_JSON_ERROR", "XML has no root element")};
    }

    std::string output = "{\n";
    indent(output, 1, std::max(1, indent_width));
    append_json_string(output, root.name());
    output += ": ";
    xml_element_to_json(root, output, 1, std::max(1, indent_width));
    output += "\n}";
    const auto json = format_json(output, std::max(1, indent_width));
    if (!json.ok) {
        return {false, {}, conversion_issue("XML_TO_JSON_ERROR", json.issue.message)};
    }
    return json;
}

FormatResult json_to_csv(const std::string& input) {
    const auto json = format_json(input, 2);
    if (!json.ok) return json;
    try {
        const YAML::Node root = YAML::Load(json.value);
        std::string output;
        if (root.IsSequence()) {
            bool all_maps = root.size() != 0;
            bool all_sequences = root.size() != 0;
            for (const auto& item : root) {
                all_maps = all_maps && item.IsMap();
                all_sequences = all_sequences && item.IsSequence();
            }
            if (all_maps) {
                std::vector<std::string> headers;
                std::unordered_map<std::string, std::size_t> positions;
                for (const auto& item : root) {
                    for (const auto& entry : item) {
                        if (!entry.first.IsScalar()) continue;
                        const std::string key = entry.first.Scalar();
                        if (positions.emplace(key, headers.size()).second) headers.push_back(key);
                    }
                }
                append_csv_row(output, headers);
                for (const auto& item : root) {
                    std::vector<std::string> row(headers.size());
                    for (const auto& entry : item) {
                        if (!entry.first.IsScalar()) continue;
                        const auto found = positions.find(entry.first.Scalar());
                        if (found != positions.end()) row[found->second] = json_node_cell(entry.second);
                    }
                    append_csv_row(output, row);
                }
            } else if (all_sequences) {
                std::size_t columns = 0;
                for (const auto& row : root) columns = std::max(columns, row.size());
                std::vector<std::string> header(columns);
                for (std::size_t i = 0; i < columns; ++i) header[i] = "column" + std::to_string(i + 1);
                append_csv_row(output, header);
                for (const auto& source : root) {
                    std::vector<std::string> row(columns);
                    for (std::size_t i = 0; i < source.size(); ++i) row[i] = json_node_cell(source[i]);
                    append_csv_row(output, row);
                }
            } else {
                append_csv_row(output, {"value"});
                for (const auto& item : root) append_csv_row(output, {json_node_cell(item)});
            }
        } else if (root.IsMap()) {
            append_csv_row(output, {"key", "value"});
            for (const auto& entry : root) {
                append_csv_row(output, {entry.first.Scalar(), json_node_cell(entry.second)});
            }
        } else {
            append_csv_row(output, {"value"});
            append_csv_row(output, {json_node_cell(root)});
        }
        if (!output.empty()) output.pop_back();
        return {true, std::move(output), {}};
    } catch (const YAML::Exception& exception) {
        return {false, {}, conversion_issue("JSON_TO_CSV_ERROR", exception.msg)};
    }
}

FormatResult yaml_to_json(const std::string& input, int indent_width) {
    const auto yaml = format_yaml(input, indent_width);
    if (!yaml.ok) return yaml;
    try {
        // Parse the original after validation so quoted scalar intent is not lost
        // through the normalization emitter (for example, "true" vs true).
        const YAML::Node root = YAML::Load(input);
        FormatResult result;
        result.ok = node_to_json(root, result.value, 0, std::max(1, indent_width), result.issue);
        return result;
    } catch (const YAML::Exception& exception) {
        return {false, {}, conversion_issue("YAML_TO_JSON_ERROR", exception.msg)};
    }
}

FormatResult unescape_json_string(const std::string& input) {
    auto decode_candidate = [](const std::string& candidate) -> FormatResult {
        const auto validated = format_json(candidate, 2);
        if (!validated.ok) return validated;
        try {
            const YAML::Node value = YAML::Load(candidate);
            if (!value.IsScalar() ||
                (value.Tag() != "!" && value.Tag() != "tag:yaml.org,2002:str")) {
                return {false, {}, conversion_issue("JSON_STRING_REQUIRED",
                    "Input is not a JSON string")};
            }
            return {true, value.Scalar(), {}};
        } catch (const YAML::Exception& exception) {
            return {false, {}, conversion_issue("INVALID_JSON_STRING", exception.msg)};
        }
    };

    const auto first = input.find_first_not_of(" \t\r\n");
    const auto last = input.find_last_not_of(" \t\r\n");
    if (first != std::string::npos && last != std::string::npos &&
        input[first] == '"' && input[last] == '"') {
        return decode_candidate(input.substr(first, last - first + 1));
    }

    std::string wrapped;
    wrapped.reserve(input.size() + 2);
    wrapped.push_back('"');
    wrapped += input;
    wrapped.push_back('"');
    return decode_candidate(wrapped);
}

} // namespace zeus
