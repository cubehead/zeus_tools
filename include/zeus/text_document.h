#pragma once

#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace zeus {

enum class TokenKind {
    Plain,
    Key,
    String,
    Number,
    Boolean,
    Null,
    Punctuation,
    Tag,
    Attribute,
    Comment,
};

struct TextSpan {
    std::size_t start = 0;
    std::size_t length = 0;
    TokenKind kind = TokenKind::Plain;
};

struct TextLine {
    std::string text;
    std::vector<TextSpan> spans;
};

struct SearchMatch {
    std::size_t offset = 0;
    std::size_t length = 0;
    std::size_t line = 0;
    std::size_t column = 0;
};

struct FoldRegion {
    std::size_t start_line = 0;
    std::size_t end_line = 0;
};

class HighlightedDocument {
public:
    static HighlightedDocument json(std::string text);
    static HighlightedDocument xml(std::string text);
    static HighlightedDocument yaml(std::string text);
    static HighlightedDocument toml(std::string text);
    static HighlightedDocument plain(std::string text);

    const std::string& text() const { return text_; }
    const std::vector<TextLine>& lines() const { return lines_; }
    const std::vector<FoldRegion>& fold_regions() const { return fold_regions_; }
    const FoldRegion* fold_region_at(std::size_t line) const;
    std::size_t offset_at(std::size_t line, std::size_t byte_column) const;
    std::pair<std::size_t, std::size_t> line_range(std::size_t line) const;

    std::vector<SearchMatch> search(
        const std::string& query,
        bool case_sensitive = false,
        bool use_regex = false,
        std::string* error = nullptr) const;

private:
    std::string text_;
    std::vector<TextLine> lines_;
    std::vector<std::size_t> line_starts_;
    std::vector<FoldRegion> fold_regions_;
};

class TextFoldState {
public:
    void ensure_document(const HighlightedDocument& document);
    void clear();
    bool toggle(const HighlightedDocument& document, std::size_t start_line);
    bool is_collapsed(std::size_t start_line) const;
    bool reveal(const HighlightedDocument& document, std::size_t line);
    std::vector<std::size_t> visible_lines(const HighlightedDocument& document) const;
    std::size_t visible_index(const HighlightedDocument& document, std::size_t line) const;
    std::size_t revision() const { return revision_; }

private:
    const HighlightedDocument* document_ = nullptr;
    std::set<std::size_t> collapsed_;
    std::size_t revision_ = 0;
};

} // namespace zeus
