#include "zeus/text_document.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <regex>
#include <utility>

namespace zeus {
namespace {

bool starts_with_at(const std::string& text, std::size_t offset, const char* value) {
    const std::string needle(value);
    return text.compare(offset, needle.size(), needle) == 0;
}

void push_span(TextLine& line, std::size_t start, std::size_t length, TokenKind kind) {
    if (length != 0) {
        line.spans.push_back({start, length, kind});
    }
}

std::string lower_ascii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

void sort_fold_regions(std::vector<FoldRegion>& regions) {
    std::sort(regions.begin(), regions.end(), [](const FoldRegion& left, const FoldRegion& right) {
        if (left.start_line != right.start_line) return left.start_line < right.start_line;
        return left.end_line > right.end_line;
    });
}

std::vector<FoldRegion> json_fold_regions(const std::vector<TextLine>& lines) {
    std::vector<std::pair<char, std::size_t>> stack;
    std::vector<FoldRegion> regions;
    bool in_string = false;
    bool escaped = false;
    for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
        for (const char ch : lines[line_index].text) {
            if (in_string) {
                if (escaped) escaped = false;
                else if (ch == '\\') escaped = true;
                else if (ch == '"') in_string = false;
                continue;
            }
            if (ch == '"') {
                in_string = true;
            } else if (ch == '{' || ch == '[') {
                stack.emplace_back(ch, line_index);
            } else if (ch == '}' || ch == ']') {
                const char expected = ch == '}' ? '{' : '[';
                if (!stack.empty() && stack.back().first == expected) {
                    const std::size_t start = stack.back().second;
                    stack.pop_back();
                    if (line_index > start) regions.push_back({start, line_index});
                }
            }
        }
    }
    sort_fold_regions(regions);
    return regions;
}

std::vector<FoldRegion> toml_fold_regions(const std::vector<TextLine>& lines) {
    std::vector<std::size_t> headers;
    for (std::size_t line = 0; line < lines.size(); ++line) {
        const std::string& text = lines[line].text;
        const std::size_t first = text.find_first_not_of(" \t");
        if (first != std::string::npos && text[first] == '[') headers.push_back(line);
    }
    std::vector<FoldRegion> regions;
    for (std::size_t index = 0; index < headers.size(); ++index) {
        const std::size_t end = index + 1 < headers.size()
            ? headers[index + 1] - 1 : lines.size() - 1;
        if (end > headers[index]) regions.push_back({headers[index], end});
    }
    return regions;
}

std::vector<FoldRegion> xml_fold_regions(const std::vector<TextLine>& lines) {
    struct OpenTag { std::string name; std::size_t line = 0; };
    std::vector<OpenTag> stack;
    std::vector<FoldRegion> regions;
    bool in_comment = false;
    for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
        const std::string& line = lines[line_index].text;
        std::size_t offset = 0;
        while (offset < line.size()) {
            if (in_comment) {
                const auto end = line.find("-->", offset);
                if (end == std::string::npos) break;
                in_comment = false;
                offset = end + 3;
                continue;
            }
            const auto open = line.find('<', offset);
            if (open == std::string::npos) break;
            if (line.compare(open, 4, "<!--") == 0) {
                const auto end = line.find("-->", open + 4);
                if (end == std::string::npos) {
                    in_comment = true;
                    break;
                }
                offset = end + 3;
                continue;
            }
            bool quoted = false;
            char quote = 0;
            std::size_t close = open + 1;
            for (; close < line.size(); ++close) {
                const char ch = line[close];
                if (quoted) {
                    if (ch == quote) quoted = false;
                } else if (ch == '"' || ch == '\'') {
                    quoted = true;
                    quote = ch;
                } else if (ch == '>') {
                    break;
                }
            }
            if (close == line.size()) break;
            std::size_t name_start = open + 1;
            const bool closing = name_start < close && line[name_start] == '/';
            if (closing) ++name_start;
            if (name_start >= close || line[name_start] == '?' || line[name_start] == '!') {
                offset = close + 1;
                continue;
            }
            std::size_t name_end = name_start;
            while (name_end < close && !std::isspace(static_cast<unsigned char>(line[name_end])) &&
                   line[name_end] != '/' && line[name_end] != '>') ++name_end;
            const std::string name = line.substr(name_start, name_end - name_start);
            std::size_t before_close = close;
            while (before_close > open &&
                   std::isspace(static_cast<unsigned char>(line[before_close - 1]))) --before_close;
            const bool self_closing = before_close > open && line[before_close - 1] == '/';
            if (closing) {
                for (std::size_t index = stack.size(); index > 0; --index) {
                    if (stack[index - 1].name != name) continue;
                    const std::size_t start = stack[index - 1].line;
                    stack.resize(index - 1);
                    if (line_index > start) regions.push_back({start, line_index});
                    break;
                }
            } else if (!self_closing) {
                stack.push_back({name, line_index});
            }
            offset = close + 1;
        }
    }
    sort_fold_regions(regions);
    return regions;
}

std::vector<FoldRegion> yaml_fold_regions(const std::vector<TextLine>& lines) {
    struct MeaningfulLine { std::size_t line = 0; std::size_t indent = 0; };
    std::vector<MeaningfulLine> meaningful;
    for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
        const std::string& line = lines[line_index].text;
        std::size_t first = 0;
        std::size_t indent = 0;
        while (first < line.size() && (line[first] == ' ' || line[first] == '\t')) {
            indent += line[first] == '\t' ? 2 : 1;
            ++first;
        }
        if (first == line.size() || line[first] == '#') continue;
        meaningful.push_back({line_index, indent});
    }
    std::vector<FoldRegion> regions;
    for (std::size_t index = 0; index + 1 < meaningful.size(); ++index) {
        if (meaningful[index + 1].indent <= meaningful[index].indent) continue;
        std::size_t end = meaningful.back().line;
        for (std::size_t next = index + 1; next < meaningful.size(); ++next) {
            if (next > index + 1 && meaningful[next].indent <= meaningful[index].indent) {
                end = meaningful[next - 1].line;
                break;
            }
        }
        if (end > meaningful[index].line) regions.push_back({meaningful[index].line, end});
    }
    sort_fold_regions(regions);
    return regions;
}

} // namespace

HighlightedDocument HighlightedDocument::json(std::string text) {
    HighlightedDocument document;
    document.text_ = std::move(text);

    std::size_t line_start = 0;
    while (line_start <= document.text_.size()) {
        document.line_starts_.push_back(line_start);
        const std::size_t line_end = document.text_.find('\n', line_start);
        const std::size_t end = line_end == std::string::npos ? document.text_.size() : line_end;
        TextLine line;
        line.text = document.text_.substr(line_start, end - line_start);

        std::size_t i = 0;
        while (i < line.text.size()) {
            const unsigned char current = static_cast<unsigned char>(line.text[i]);
            if (std::isspace(current)) {
                const std::size_t start = i++;
                while (i < line.text.size() && std::isspace(static_cast<unsigned char>(line.text[i]))) {
                    ++i;
                }
                push_span(line, start, i - start, TokenKind::Plain);
            } else if (line.text[i] == '"') {
                const std::size_t start = i++;
                bool escaped = false;
                while (i < line.text.size()) {
                    const char ch = line.text[i++];
                    if (escaped) {
                        escaped = false;
                    } else if (ch == '\\') {
                        escaped = true;
                    } else if (ch == '"') {
                        break;
                    }
                }
                std::size_t next = i;
                while (next < line.text.size() && std::isspace(static_cast<unsigned char>(line.text[next]))) {
                    ++next;
                }
                push_span(line, start, i - start,
                          next < line.text.size() && line.text[next] == ':' ? TokenKind::Key : TokenKind::String);
            } else if (line.text[i] == '-' || std::isdigit(current)) {
                const std::size_t start = i++;
                while (i < line.text.size()) {
                    const char ch = line.text[i];
                    if (!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '.' || ch == 'e' || ch == 'E' || ch == '+' || ch == '-')) {
                        break;
                    }
                    ++i;
                }
                push_span(line, start, i - start, TokenKind::Number);
            } else if (starts_with_at(line.text, i, "true") || starts_with_at(line.text, i, "false")) {
                const std::size_t length = starts_with_at(line.text, i, "true") ? 4 : 5;
                push_span(line, i, length, TokenKind::Boolean);
                i += length;
            } else if (starts_with_at(line.text, i, "null")) {
                push_span(line, i, 4, TokenKind::Null);
                i += 4;
            } else if (std::string("{}[]:,").find(line.text[i]) != std::string::npos) {
                push_span(line, i, 1, TokenKind::Punctuation);
                ++i;
            } else {
                push_span(line, i, 1, TokenKind::Plain);
                ++i;
            }
        }
        document.lines_.push_back(std::move(line));
        if (line_end == std::string::npos) {
            break;
        }
        line_start = line_end + 1;
    }
    document.fold_regions_ = json_fold_regions(document.lines_);
    return document;
}

HighlightedDocument HighlightedDocument::xml(std::string text) {
    HighlightedDocument document;
    document.text_ = std::move(text);

    bool in_comment = false;
    bool in_cdata = false;
    std::size_t line_start = 0;
    while (line_start <= document.text_.size()) {
        document.line_starts_.push_back(line_start);
        const std::size_t line_end = document.text_.find('\n', line_start);
        const std::size_t end = line_end == std::string::npos ? document.text_.size() : line_end;
        TextLine line;
        line.text = document.text_.substr(line_start, end - line_start);

        bool in_tag = false;
        bool expect_tag_name = false;
        std::size_t i = 0;
        while (i < line.text.size()) {
            if (in_comment) {
                const auto close = line.text.find("-->", i);
                const auto token_end = close == std::string::npos ? line.text.size() : close + 3;
                push_span(line, i, token_end - i, TokenKind::Comment);
                i = token_end;
                in_comment = close == std::string::npos;
                continue;
            }
            if (in_cdata) {
                const auto close = line.text.find("]]>", i);
                const auto token_end = close == std::string::npos ? line.text.size() : close + 3;
                push_span(line, i, token_end - i, TokenKind::String);
                i = token_end;
                in_cdata = close == std::string::npos;
                continue;
            }
            if (!in_tag && line.text.compare(i, 4, "<!--") == 0) {
                in_comment = true;
                continue;
            }
            if (!in_tag && line.text.compare(i, 9, "<![CDATA[") == 0) {
                in_cdata = true;
                continue;
            }
            if (!in_tag && line.text[i] == '<') {
                push_span(line, i, 1, TokenKind::Punctuation);
                ++i;
                in_tag = true;
                expect_tag_name = true;
                if (i < line.text.size() && (line.text[i] == '/' || line.text[i] == '?')) {
                    push_span(line, i, 1, TokenKind::Punctuation);
                    ++i;
                }
                continue;
            }
            if (in_tag && (line.text[i] == '>' || line.text[i] == '?' || line.text[i] == '/')) {
                push_span(line, i, 1, TokenKind::Punctuation);
                if (line.text[i] == '>') in_tag = false;
                ++i;
                continue;
            }
            if (in_tag && (line.text[i] == '"' || line.text[i] == '\'')) {
                const char quote = line.text[i];
                const std::size_t start = i++;
                while (i < line.text.size() && line.text[i] != quote) ++i;
                if (i < line.text.size()) ++i;
                push_span(line, start, i - start, TokenKind::String);
                continue;
            }
            if (in_tag && line.text[i] == '=') {
                push_span(line, i++, 1, TokenKind::Punctuation);
                continue;
            }
            const unsigned char ch = static_cast<unsigned char>(line.text[i]);
            if (in_tag && !std::isspace(ch)) {
                const std::size_t start = i++;
                while (i < line.text.size()) {
                    const unsigned char next = static_cast<unsigned char>(line.text[i]);
                    if (std::isspace(next) || line.text[i] == '=' || line.text[i] == '>' ||
                        line.text[i] == '/' || line.text[i] == '?') break;
                    ++i;
                }
                push_span(line, start, i - start,
                          expect_tag_name ? TokenKind::Tag : TokenKind::Attribute);
                expect_tag_name = false;
                continue;
            }
            const std::size_t start = i++;
            while (i < line.text.size() && line.text[i] != '<' &&
                   (!in_tag || std::isspace(static_cast<unsigned char>(line.text[i])))) ++i;
            push_span(line, start, i - start, TokenKind::Plain);
        }
        document.lines_.push_back(std::move(line));
        if (line_end == std::string::npos) break;
        line_start = line_end + 1;
    }
    document.fold_regions_ = xml_fold_regions(document.lines_);
    return document;
}

HighlightedDocument HighlightedDocument::yaml(std::string text) {
    HighlightedDocument document;
    document.text_ = std::move(text);
    std::size_t line_start = 0;
    while (line_start <= document.text_.size()) {
        document.line_starts_.push_back(line_start);
        const std::size_t line_end = document.text_.find('\n', line_start);
        const std::size_t end = line_end == std::string::npos ? document.text_.size() : line_end;
        TextLine line;
        line.text = document.text_.substr(line_start, end - line_start);

        std::size_t i = 0;
        while (i < line.text.size()) {
            const unsigned char ch = static_cast<unsigned char>(line.text[i]);
            if (std::isspace(ch)) {
                const std::size_t start = i++;
                while (i < line.text.size() && std::isspace(static_cast<unsigned char>(line.text[i]))) ++i;
                push_span(line, start, i - start, TokenKind::Plain);
            } else if (line.text[i] == '#') {
                push_span(line, i, line.text.size() - i, TokenKind::Comment);
                i = line.text.size();
            } else if (line.text[i] == '"' || line.text[i] == '\'') {
                const char quote = line.text[i];
                const std::size_t start = i++;
                bool escaped = false;
                while (i < line.text.size()) {
                    const char current = line.text[i++];
                    if (quote == '"' && escaped) escaped = false;
                    else if (quote == '"' && current == '\\') escaped = true;
                    else if (current == quote) break;
                }
                push_span(line, start, i - start, TokenKind::String);
            } else if (std::string("{}[],:-").find(line.text[i]) != std::string::npos) {
                push_span(line, i++, 1, TokenKind::Punctuation);
            } else {
                const std::size_t start = i++;
                while (i < line.text.size() && !std::isspace(static_cast<unsigned char>(line.text[i])) &&
                       std::string("{}[],:#").find(line.text[i]) == std::string::npos) ++i;
                std::size_t next = i;
                while (next < line.text.size() && std::isspace(static_cast<unsigned char>(line.text[next]))) ++next;
                const std::string token = line.text.substr(start, i - start);
                const std::string lowered = lower_ascii(token);
                TokenKind kind = TokenKind::String;
                if (next < line.text.size() && line.text[next] == ':') {
                    kind = TokenKind::Key;
                } else if (lowered == "true" || lowered == "false") {
                    kind = TokenKind::Boolean;
                } else if (lowered == "null" || token == "~") {
                    kind = TokenKind::Null;
                } else {
                    char* number_end = nullptr;
                    std::strtod(token.c_str(), &number_end);
                    if (number_end != token.c_str() && *number_end == '\0') kind = TokenKind::Number;
                }
                push_span(line, start, i - start, kind);
            }
        }
        document.lines_.push_back(std::move(line));
        if (line_end == std::string::npos) break;
        line_start = line_end + 1;
    }
    document.fold_regions_ = yaml_fold_regions(document.lines_);
    return document;
}

HighlightedDocument HighlightedDocument::toml(std::string text) {
    HighlightedDocument document;
    document.text_ = std::move(text);
    std::size_t line_start = 0;
    while (line_start <= document.text_.size()) {
        document.line_starts_.push_back(line_start);
        const std::size_t line_end = document.text_.find('\n', line_start);
        const std::size_t end = line_end == std::string::npos ? document.text_.size() : line_end;
        TextLine line;
        line.text = document.text_.substr(line_start, end - line_start);

        const std::size_t equals = line.text.find('=');
        std::size_t index = 0;
        while (index < line.text.size()) {
            const unsigned char ch = static_cast<unsigned char>(line.text[index]);
            if (std::isspace(ch)) {
                const std::size_t start = index++;
                while (index < line.text.size() &&
                       std::isspace(static_cast<unsigned char>(line.text[index]))) ++index;
                push_span(line, start, index - start, TokenKind::Plain);
            } else if (line.text[index] == '#') {
                push_span(line, index, line.text.size() - index, TokenKind::Comment);
                index = line.text.size();
            } else if (line.text[index] == '"' || line.text[index] == '\'') {
                const char quote = line.text[index];
                const bool triple = index + 2 < line.text.size() &&
                    line.text[index + 1] == quote && line.text[index + 2] == quote;
                const std::size_t start = index;
                index += triple ? 3 : 1;
                bool escaped = false;
                while (index < line.text.size()) {
                    if (triple && index + 2 < line.text.size() &&
                        line.text[index] == quote && line.text[index + 1] == quote &&
                        line.text[index + 2] == quote) {
                        index += 3;
                        break;
                    }
                    const char current = line.text[index++];
                    if (quote == '"' && escaped) escaped = false;
                    else if (quote == '"' && current == '\\') escaped = true;
                    else if (!triple && current == quote) break;
                }
                push_span(line, start, index - start, TokenKind::String);
            } else if (std::string("[]=,.{}").find(line.text[index]) != std::string::npos) {
                push_span(line, index++, 1, TokenKind::Punctuation);
            } else {
                const std::size_t start = index++;
                while (index < line.text.size() &&
                       !std::isspace(static_cast<unsigned char>(line.text[index])) &&
                       std::string("[]=,.{}#\"'").find(line.text[index]) == std::string::npos) ++index;
                const std::string token = line.text.substr(start, index - start);
                const std::string lowered = lower_ascii(token);
                TokenKind kind = equals != std::string::npos && start < equals
                    ? TokenKind::Key : TokenKind::String;
                if (lowered == "true" || lowered == "false") kind = TokenKind::Boolean;
                else {
                    char* number_end = nullptr;
                    std::strtod(token.c_str(), &number_end);
                    if (number_end != token.c_str() && *number_end == '\0') kind = TokenKind::Number;
                }
                push_span(line, start, index - start, kind);
            }
        }
        document.lines_.push_back(std::move(line));
        if (line_end == std::string::npos) break;
        line_start = line_end + 1;
    }
    document.fold_regions_ = toml_fold_regions(document.lines_);
    return document;
}

HighlightedDocument HighlightedDocument::plain(std::string text) {
    HighlightedDocument document;
    document.text_ = std::move(text);
    std::size_t line_start = 0;
    while (line_start <= document.text_.size()) {
        document.line_starts_.push_back(line_start);
        const std::size_t line_end = document.text_.find('\n', line_start);
        const std::size_t end = line_end == std::string::npos ? document.text_.size() : line_end;
        TextLine line;
        line.text = document.text_.substr(line_start, end - line_start);
        push_span(line, 0, line.text.size(), TokenKind::Plain);
        document.lines_.push_back(std::move(line));
        if (line_end == std::string::npos) break;
        line_start = line_end + 1;
    }
    return document;
}

std::size_t HighlightedDocument::offset_at(std::size_t line, std::size_t byte_column) const {
    if (lines_.empty()) {
        return 0;
    }
    line = std::min(line, lines_.size() - 1);
    byte_column = std::min(byte_column, lines_[line].text.size());
    std::size_t offset = line_starts_[line] + byte_column;
    while (offset > line_starts_[line] && offset < text_.size() &&
           (static_cast<unsigned char>(text_[offset]) & 0xC0U) == 0x80U) {
        --offset;
    }
    return offset;
}

std::pair<std::size_t, std::size_t> HighlightedDocument::line_range(std::size_t line) const {
    if (lines_.empty()) {
        return {0, 0};
    }
    line = std::min(line, lines_.size() - 1);
    const std::size_t start = line_starts_[line];
    return {start, start + lines_[line].text.size()};
}

const FoldRegion* HighlightedDocument::fold_region_at(std::size_t line) const {
    const auto it = std::lower_bound(
        fold_regions_.begin(), fold_regions_.end(), line,
        [](const FoldRegion& region, std::size_t value) { return region.start_line < value; });
    return it != fold_regions_.end() && it->start_line == line ? &*it : nullptr;
}

void TextFoldState::ensure_document(const HighlightedDocument& document) {
    if (document_ == &document) return;
    document_ = &document;
    collapsed_.clear();
    ++revision_;
}

void TextFoldState::clear() {
    document_ = nullptr;
    collapsed_.clear();
    ++revision_;
}

bool TextFoldState::toggle(const HighlightedDocument& document, std::size_t start_line) {
    ensure_document(document);
    if (document.fold_region_at(start_line) == nullptr) return false;
    return set_collapsed(document, start_line, !is_collapsed(start_line));
}

bool TextFoldState::set_collapsed(
    const HighlightedDocument& document,
    std::size_t start_line,
    bool collapsed) {
    ensure_document(document);
    if (document.fold_region_at(start_line) == nullptr ||
        is_collapsed(start_line) == collapsed) {
        return false;
    }
    if (collapsed) collapsed_.insert(start_line);
    else collapsed_.erase(start_line);
    ++revision_;
    return true;
}

bool TextFoldState::is_collapsed(std::size_t start_line) const {
    return collapsed_.find(start_line) != collapsed_.end();
}

bool TextFoldState::reveal(const HighlightedDocument& document, std::size_t line) {
    ensure_document(document);
    bool changed = false;
    for (const auto& region : document.fold_regions()) {
        if (region.start_line < line && line <= region.end_line) {
            changed = collapsed_.erase(region.start_line) != 0 || changed;
        }
    }
    if (changed) ++revision_;
    return changed;
}

std::vector<std::size_t> TextFoldState::visible_lines(const HighlightedDocument& document) const {
    std::vector<std::size_t> visible;
    visible.reserve(document.lines().size());
    std::size_t line = 0;
    while (line < document.lines().size()) {
        visible.push_back(line);
        const FoldRegion* region = document.fold_region_at(line);
        if (region != nullptr && is_collapsed(line)) line = region->end_line + 1;
        else ++line;
    }
    return visible;
}

std::size_t TextFoldState::visible_index(
    const HighlightedDocument& document,
    std::size_t line) const {
    const auto visible = visible_lines(document);
    const auto it = std::lower_bound(visible.begin(), visible.end(), line);
    if (it == visible.end()) return visible.empty() ? 0 : visible.size() - 1;
    return static_cast<std::size_t>(std::distance(visible.begin(), it));
}

std::vector<SearchMatch> HighlightedDocument::search(
    const std::string& query,
    bool case_sensitive,
    bool use_regex,
    std::string* error) const {
    if (error != nullptr) {
        error->clear();
    }
    std::vector<SearchMatch> matches;
    if (query.empty()) {
        return matches;
    }

    auto append_match = [&](std::size_t offset, std::size_t length) {
        const auto line_it = std::upper_bound(line_starts_.begin(), line_starts_.end(), offset);
        const std::size_t line = line_it == line_starts_.begin()
            ? 0
            : static_cast<std::size_t>(std::distance(line_starts_.begin(), line_it) - 1);
        const std::size_t column = offset - line_starts_[line];
        matches.push_back({offset, length, line, column});
    };

    if (use_regex) {
        try {
            const auto flags = case_sensitive ? std::regex::ECMAScript : (std::regex::ECMAScript | std::regex::icase);
            const std::regex expression(query, flags);
            for (std::sregex_iterator it(text_.begin(), text_.end(), expression), end; it != end; ++it) {
                append_match(static_cast<std::size_t>(it->position()), static_cast<std::size_t>(it->length()));
            }
        } catch (const std::regex_error& exception) {
            if (error != nullptr) {
                *error = exception.what();
            }
        }
        return matches;
    }

    const std::string haystack = case_sensitive ? text_ : lower_ascii(text_);
    const std::string needle = case_sensitive ? query : lower_ascii(query);
    std::size_t position = 0;
    while ((position = haystack.find(needle, position)) != std::string::npos) {
        append_match(position, needle.size());
        position += std::max<std::size_t>(needle.size(), 1);
    }
    return matches;
}

} // namespace zeus
