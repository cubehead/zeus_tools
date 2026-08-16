#include "zeus/csv_document.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <regex>

namespace zeus {
namespace {

CsvParseResult parse_with_delimiter(const std::string& input, char delimiter) {
    CsvParseResult result;
    result.document.delimiter = delimiter;
    std::vector<std::string> row;
    std::string field;
    bool quoted = false;
    bool after_quote = false;

    auto finish_field = [&] {
        row.push_back(std::move(field));
        field.clear();
        after_quote = false;
    };
    auto finish_row = [&] {
        finish_field();
        result.document.rows.push_back(std::move(row));
        row.clear();
    };

    for (std::size_t i = 0; i < input.size(); ++i) {
        const char ch = input[i];
        if (quoted) {
            if (ch == '"') {
                if (i + 1 < input.size() && input[i + 1] == '"') {
                    field.push_back('"');
                    ++i;
                } else {
                    quoted = false;
                    after_quote = true;
                }
            } else {
                field.push_back(ch);
            }
            continue;
        }
        if (after_quote && ch != delimiter && ch != '\r' && ch != '\n') {
            result.error = "Unexpected character after closing quote";
            return result;
        }
        if (ch == '"' && field.empty()) {
            quoted = true;
        } else if (ch == delimiter) {
            finish_field();
        } else if (ch == '\n' || ch == '\r') {
            if (ch == '\r' && i + 1 < input.size() && input[i + 1] == '\n') ++i;
            finish_row();
        } else {
            field.push_back(ch);
        }
    }
    if (quoted) {
        result.error = "Unterminated quoted field";
        return result;
    }
    if (!field.empty() || !row.empty() || (!input.empty() && input.back() == delimiter)) finish_row();
    while (!result.document.rows.empty() && result.document.rows.back().size() == 1 &&
           result.document.rows.back().front().empty()) {
        result.document.rows.pop_back();
    }
    result.ok = !result.document.rows.empty();
    return result;
}

double table_score(const CsvDocument& document) {
    if (document.rows.size() < 2) return 0.0;
    std::size_t widest = 0;
    for (const auto& row : document.rows) widest = std::max(widest, row.size());
    if (widest < 2) return 0.0;
    const std::size_t sample = std::min<std::size_t>(document.rows.size(), 100);
    std::size_t consistent = 0;
    for (std::size_t i = 0; i < sample; ++i) {
        if (document.rows[i].size() == widest) ++consistent;
    }
    return static_cast<double>(consistent) / static_cast<double>(sample) +
           std::min<double>(static_cast<double>(widest), 10.0) * 0.01;
}

} // namespace

std::string CsvDocument::to_tsv() const {
    std::size_t output_bytes = rows.empty() ? 0 : rows.size() - 1;
    for (const auto& row : rows) {
        if (!row.empty()) output_bytes += row.size() - 1;
        for (const auto& cell : row) output_bytes += cell.size();
    }

    std::string output;
    output.reserve(output_bytes);
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        if (row_index != 0) output.push_back('\n');
        for (std::size_t column = 0; column < rows[row_index].size(); ++column) {
            if (column != 0) output.push_back('\t');
            for (const char value : rows[row_index][column]) {
                output.push_back(value == '\t' || value == '\n' || value == '\r'
                    ? ' ' : value);
            }
        }
    }
    return output;
}

std::vector<CsvSearchMatch> CsvDocument::search(
    const std::string& query,
    bool case_sensitive,
    bool use_regex,
    std::string* error) const {
    if (error != nullptr) error->clear();
    std::vector<CsvSearchMatch> matches;
    if (query.empty()) return matches;

    std::regex expression;
    if (use_regex) {
        try {
            const auto flags = case_sensitive
                ? std::regex::ECMAScript
                : (std::regex::ECMAScript | std::regex::icase);
            expression = std::regex(query, flags);
        } catch (const std::regex_error& exception) {
            if (error != nullptr) *error = exception.what();
            return matches;
        }
    }

    const auto equal_ascii_ci = [](unsigned char left, unsigned char right) {
        return std::tolower(left) == std::tolower(right);
    };
    for (std::size_t row = 0; row < rows.size(); ++row) {
        for (std::size_t column = 0; column < rows[row].size(); ++column) {
            const auto& cell = rows[row][column];
            if (use_regex) {
                for (auto it = std::sregex_iterator(cell.begin(), cell.end(), expression);
                     it != std::sregex_iterator(); ++it) {
                    const auto start = static_cast<std::size_t>(it->position());
                    const auto length = static_cast<std::size_t>(it->length());
                    if (length > 0) matches.push_back({row, column, start, length});
                }
                continue;
            }

            std::size_t offset = 0;
            while (offset <= cell.size()) {
                std::size_t start = std::string::npos;
                if (case_sensitive) {
                    start = cell.find(query, offset);
                } else {
                    const auto found = std::search(
                        cell.begin() + static_cast<std::ptrdiff_t>(offset), cell.end(),
                        query.begin(), query.end(), equal_ascii_ci);
                    if (found != cell.end()) {
                        start = static_cast<std::size_t>(std::distance(cell.begin(), found));
                    }
                }
                if (start == std::string::npos) break;
                matches.push_back({row, column, start, query.size()});
                offset = start + query.size();
            }
        }
    }
    return matches;
}

CsvParseResult parse_csv(const std::string& input, char delimiter, bool require_table) {
    if (delimiter != '\0') {
        auto result = parse_with_delimiter(input, delimiter);
        if (result.ok && require_table && table_score(result.document) < 0.8) {
            result.ok = false;
            result.error = "Input does not contain a consistent table";
        }
        return result;
    }

    if (input.find('\n') == std::string::npos && input.find('\r') == std::string::npos) {
        CsvParseResult result;
        result.error = "CSV auto-detection requires at least two rows";
        return result;
    }

    constexpr std::array<char, 4> candidates{',', '\t', ';', '|'};
    CsvParseResult best;
    double best_score = 0.0;
    for (char candidate : candidates) {
        if (input.find(candidate) == std::string::npos) continue;
        auto parsed = parse_with_delimiter(input, candidate);
        if (!parsed.ok) continue;
        const double score = table_score(parsed.document);
        if (score > best_score) {
            best_score = score;
            best = std::move(parsed);
        }
    }
    if (best_score < 0.8) {
        best.ok = false;
        best.error = "Could not detect a consistent CSV delimiter";
    }
    return best;
}

} // namespace zeus
