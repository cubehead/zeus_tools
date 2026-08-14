#include "zeus/xml_formatter.h"

#include <pugixml.hpp>

#include <algorithm>
#include <cctype>
#include <string>

namespace zeus {
namespace {

std::string lower_ascii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

void append_escaped(std::string& output, const char* value, bool attribute) {
    for (const unsigned char ch : std::string(value)) {
        switch (ch) {
        case '&': output += "&amp;"; break;
        case '<': output += "&lt;"; break;
        case '>': output += "&gt;"; break;
        case '"': output += attribute ? "&quot;" : "\""; break;
        default: output.push_back(static_cast<char>(ch)); break;
        }
    }
}

void indent(std::string& output, int depth, int width) {
    output.append(static_cast<std::size_t>(std::max(0, depth * width)), ' ');
}

bool has_mixed_text(const pugi::xml_node& node) {
    for (const auto& child : node.children()) {
        if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata) return true;
    }
    return false;
}

void serialize_node(const pugi::xml_node& node, std::string& output,
                    int depth, int width, bool pretty) {
    switch (node.type()) {
    case pugi::node_declaration:
        output += "<?";
        output += node.name()[0] == '\0' ? "xml" : node.name();
        for (const auto& attribute : node.attributes()) {
            output.push_back(' ');
            output += attribute.name();
            output += "=\"";
            append_escaped(output, attribute.value(), true);
            output.push_back('"');
        }
        output += "?>";
        break;
    case pugi::node_element: {
        output.push_back('<');
        output += node.name();
        for (const auto& attribute : node.attributes()) {
            output.push_back(' ');
            output += attribute.name();
            output += "=\"";
            append_escaped(output, attribute.value(), true);
            output.push_back('"');
        }
        if (!node.first_child()) {
            output += " />";
            break;
        }
        output.push_back('>');
        const bool mixed = has_mixed_text(node);
        if (pretty && !mixed) output.push_back('\n');
        for (auto child = node.first_child(); child; child = child.next_sibling()) {
            if (pretty && !mixed) indent(output, depth + 1, width);
            serialize_node(child, output, depth + 1, width, pretty && !mixed);
            if (pretty && !mixed) output.push_back('\n');
        }
        if (pretty && !mixed) indent(output, depth, width);
        output += "</";
        output += node.name();
        output.push_back('>');
        break;
    }
    case pugi::node_pcdata:
        append_escaped(output, node.value(), false);
        break;
    case pugi::node_cdata:
        output += "<![CDATA[";
        output += node.value();
        output += "]]>";
        break;
    case pugi::node_comment:
        output += "<!--";
        output += node.value();
        output += "-->";
        break;
    case pugi::node_pi:
        output += "<?";
        output += node.name();
        if (node.value()[0] != '\0') {
            output.push_back(' ');
            output += node.value();
        }
        output += "?>";
        break;
    default:
        break;
    }
}

ParseIssue issue_at(const std::string& input, const char* code, const std::string& message,
                    std::size_t offset) {
    ParseIssue issue;
    issue.code = code;
    issue.message = message;
    issue.offset = std::min(offset, input.size());
    for (std::size_t i = 0; i < issue.offset; ++i) {
        if (input[i] == '\n') {
            ++issue.line;
            issue.column = 1;
        } else {
            ++issue.column;
        }
    }
    return issue;
}

} // namespace

FormatResult format_xml(const std::string& input, int indent_width) {
    const std::string lowered = lower_ascii(input);
    const auto unsafe = std::min(
        lowered.find("<!doctype"), lowered.find("<!entity"));
    if (unsafe != std::string::npos) {
        return {false, {}, issue_at(input, "XML_UNSAFE_DECLARATION",
            "DOCTYPE and ENTITY declarations are disabled", unsafe)};
    }

    pugi::xml_document document;
    const unsigned int flags = pugi::parse_default | pugi::parse_comments |
                               pugi::parse_pi | pugi::parse_declaration;
    const pugi::xml_parse_result parsed = document.load_buffer(
        input.data(), input.size(), flags, pugi::encoding_utf8);
    if (!parsed) {
        const std::size_t offset = parsed.offset < 0 ? 0 : static_cast<std::size_t>(parsed.offset);
        return {false, {}, issue_at(input, "INVALID_XML", parsed.description(), offset)};
    }

    std::size_t root_count = 0;
    for (const auto& child : document.children()) {
        if (child.type() == pugi::node_element) ++root_count;
    }
    if (root_count != 1) {
        return {false, {}, issue_at(input, "INVALID_XML_ROOT",
            "XML must contain exactly one root element", 0)};
    }

    FormatResult result;
    result.ok = true;
    bool first = true;
    for (const auto& child : document.children()) {
        if (child.type() != pugi::node_declaration && child.type() != pugi::node_element &&
            child.type() != pugi::node_comment && child.type() != pugi::node_pi) continue;
        if (!first) result.value.push_back('\n');
        serialize_node(child, result.value, 0, std::max(0, indent_width), true);
        first = false;
    }
    return result;
}

} // namespace zeus
