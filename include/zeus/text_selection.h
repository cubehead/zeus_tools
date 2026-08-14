#pragma once

#include "zeus/text_document.h"

#include <cstddef>
#include <string>
#include <utility>

namespace zeus {

std::size_t selection_line_at_viewport_position(
    std::size_t line_count,
    float row_height,
    float scroll_offset,
    float viewport_y);

class TextSelection {
public:
    void clear();
    void begin(const HighlightedDocument& document, std::size_t line, std::size_t byte_column);
    void extend(const HighlightedDocument& document, std::size_t line, std::size_t byte_column);
    void select_word(const HighlightedDocument& document, std::size_t line, std::size_t byte_column);
    void select_line(const HighlightedDocument& document, std::size_t line);
    void select_lines(const HighlightedDocument& document, std::size_t first_line, std::size_t last_line);
    void select_all(const HighlightedDocument& document);

    bool empty() const { return anchor_ == caret_; }
    std::size_t selected_length() const;
    std::pair<std::size_t, std::size_t> range() const;
    std::pair<std::size_t, std::size_t> columns_for_line(
        const HighlightedDocument& document,
        std::size_t line) const;
    std::string selected_text(const HighlightedDocument& document) const;
    bool includes_line_break_after(const HighlightedDocument& document, std::size_t line) const;

private:
    std::size_t anchor_ = 0;
    std::size_t caret_ = 0;
};

} // namespace zeus
