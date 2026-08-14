#include "zeus/text_selection.h"

#include <algorithm>
#include <cctype>

namespace zeus {
namespace {

enum class CharacterClass {
    Whitespace,
    Word,
    Punctuation,
};

unsigned int codepoint_at(const std::string& text, std::size_t offset) {
    const unsigned char first = static_cast<unsigned char>(text[offset]);
    if (first < 0x80U) return first;
    if ((first & 0xE0U) == 0xC0U && offset + 1 < text.size()) {
        return ((first & 0x1FU) << 6U) |
               (static_cast<unsigned char>(text[offset + 1]) & 0x3FU);
    }
    if ((first & 0xF0U) == 0xE0U && offset + 2 < text.size()) {
        return ((first & 0x0FU) << 12U) |
               ((static_cast<unsigned char>(text[offset + 1]) & 0x3FU) << 6U) |
               (static_cast<unsigned char>(text[offset + 2]) & 0x3FU);
    }
    if ((first & 0xF8U) == 0xF0U && offset + 3 < text.size()) {
        return ((first & 0x07U) << 18U) |
               ((static_cast<unsigned char>(text[offset + 1]) & 0x3FU) << 12U) |
               ((static_cast<unsigned char>(text[offset + 2]) & 0x3FU) << 6U) |
               (static_cast<unsigned char>(text[offset + 3]) & 0x3FU);
    }
    return first;
}

CharacterClass character_class(const std::string& text, std::size_t offset) {
    const unsigned char ch = static_cast<unsigned char>(text[offset]);
    if (ch < 0x80U) {
        if (std::isalnum(ch)) return CharacterClass::Word;
        if (std::isspace(ch)) return CharacterClass::Whitespace;
        return CharacterClass::Punctuation;
    }

    const unsigned int codepoint = codepoint_at(text, offset);
    if (codepoint == 0x00A0U || codepoint == 0x3000U ||
        (codepoint >= 0x2000U && codepoint <= 0x200AU)) {
        return CharacterClass::Whitespace;
    }
    const bool punctuation_or_symbol =
        (codepoint >= 0x200BU && codepoint <= 0x206FU) ||
        (codepoint >= 0x2190U && codepoint <= 0x2BFFU) ||
        (codepoint >= 0x3001U && codepoint <= 0x303FU) ||
        (codepoint >= 0xFE10U && codepoint <= 0xFE4FU) ||
        (codepoint >= 0xFF01U && codepoint <= 0xFF0FU) ||
        (codepoint >= 0xFF1AU && codepoint <= 0xFF20U) ||
        (codepoint >= 0xFF3BU && codepoint <= 0xFF40U) ||
        (codepoint >= 0xFF5BU && codepoint <= 0xFF65U) ||
        (codepoint >= 0x1F000U && codepoint <= 0x1FAFFU);
    return punctuation_or_symbol ? CharacterClass::Punctuation : CharacterClass::Word;
}

std::size_t previous_codepoint(const std::string& text, std::size_t offset) {
    if (offset == 0) return 0;
    --offset;
    while (offset > 0 && (static_cast<unsigned char>(text[offset]) & 0xC0U) == 0x80U) {
        --offset;
    }
    return offset;
}

std::size_t next_codepoint(const std::string& text, std::size_t offset) {
    if (offset >= text.size()) return text.size();
    ++offset;
    while (offset < text.size() &&
           (static_cast<unsigned char>(text[offset]) & 0xC0U) == 0x80U) {
        ++offset;
    }
    return offset;
}

} // namespace

std::size_t selection_line_at_viewport_position(
    std::size_t line_count,
    float row_height,
    float scroll_offset,
    float viewport_y) {
    if (line_count == 0 || row_height <= 0.0f) {
        return 0;
    }
    const float content_y = std::max(0.0f, scroll_offset + std::max(0.0f, viewport_y));
    const auto line = static_cast<std::size_t>(content_y / row_height);
    return std::min(line, line_count - 1);
}

void TextSelection::clear() {
    anchor_ = 0;
    caret_ = 0;
}

void TextSelection::begin(
    const HighlightedDocument& document,
    std::size_t line,
    std::size_t byte_column) {
    anchor_ = document.offset_at(line, byte_column);
    caret_ = anchor_;
}

void TextSelection::extend(
    const HighlightedDocument& document,
    std::size_t line,
    std::size_t byte_column) {
    caret_ = document.offset_at(line, byte_column);
}

void TextSelection::select_word(
    const HighlightedDocument& document,
    std::size_t line,
    std::size_t byte_column) {
    if (document.lines().empty()) {
        clear();
        return;
    }
    line = std::min(line, document.lines().size() - 1);
    const std::string& text = document.lines()[line].text;
    if (text.empty()) {
        begin(document, line, 0);
        return;
    }

    std::size_t offset = std::min(byte_column, text.size());
    if (offset == text.size()) offset = previous_codepoint(text, offset);
    while (offset > 0 && offset < text.size() &&
           (static_cast<unsigned char>(text[offset]) & 0xC0U) == 0x80U) {
        --offset;
    }
    const CharacterClass selected_class = character_class(text, offset);
    if (selected_class == CharacterClass::Punctuation) {
        anchor_ = document.offset_at(line, offset);
        caret_ = document.offset_at(line, next_codepoint(text, offset));
        return;
    }
    std::size_t start = offset;
    while (start > 0) {
        const std::size_t previous = previous_codepoint(text, start);
        if (character_class(text, previous) != selected_class) break;
        start = previous;
    }
    std::size_t end = next_codepoint(text, offset);
    while (end < text.size() && character_class(text, end) == selected_class) {
        end = next_codepoint(text, end);
    }
    anchor_ = document.offset_at(line, start);
    caret_ = document.offset_at(line, end);
}

void TextSelection::select_line(const HighlightedDocument& document, std::size_t line) {
    select_lines(document, line, line);
}

void TextSelection::select_lines(
    const HighlightedDocument& document,
    std::size_t first_line,
    std::size_t last_line) {
    if (document.lines().empty()) {
        clear();
        return;
    }
    const auto ordered = std::minmax(first_line, last_line);
    const std::size_t first = std::min(ordered.first, document.lines().size() - 1);
    const std::size_t last = std::min(ordered.second, document.lines().size() - 1);
    anchor_ = document.line_range(first).first;
    caret_ = document.line_range(last).second;
    if (caret_ < document.text().size() && document.text()[caret_] == '\n') {
        ++caret_;
    }
}

void TextSelection::select_all(const HighlightedDocument& document) {
    anchor_ = 0;
    caret_ = document.text().size();
}

std::pair<std::size_t, std::size_t> TextSelection::range() const {
    return std::minmax(anchor_, caret_);
}

std::size_t TextSelection::selected_length() const {
    const auto selected = range();
    return selected.second - selected.first;
}

std::pair<std::size_t, std::size_t> TextSelection::columns_for_line(
    const HighlightedDocument& document,
    std::size_t line) const {
    const auto selected = range();
    const auto line_offsets = document.line_range(line);
    const std::size_t start = std::max(selected.first, line_offsets.first);
    const std::size_t end = std::min(selected.second, line_offsets.second);
    if (start >= end) {
        return {0, 0};
    }
    return {start - line_offsets.first, end - line_offsets.first};
}

std::string TextSelection::selected_text(const HighlightedDocument& document) const {
    const auto selected = range();
    if (selected.first >= selected.second || selected.second > document.text().size()) {
        return {};
    }
    return document.text().substr(selected.first, selected.second - selected.first);
}

bool TextSelection::includes_line_break_after(
    const HighlightedDocument& document,
    std::size_t line) const {
    const auto selected = range();
    const auto line_offsets = document.line_range(line);
    return line_offsets.second < document.text().size() &&
           document.text()[line_offsets.second] == '\n' &&
           selected.first <= line_offsets.second && selected.second > line_offsets.second;
}

} // namespace zeus
