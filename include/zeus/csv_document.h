#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace zeus {

struct CsvSearchMatch {
    std::size_t row = 0;
    std::size_t column = 0;
    std::size_t start = 0;
    std::size_t length = 0;
};

struct CsvDocument {
    char delimiter = ',';
    std::vector<std::vector<std::string>> rows;

    std::string to_tsv() const;
    std::vector<CsvSearchMatch> search(
        const std::string& query,
        bool case_sensitive = false,
        bool use_regex = false,
        std::string* error = nullptr) const;
};

struct CsvParseResult {
    bool ok = false;
    CsvDocument document;
    std::string error;
};

CsvParseResult parse_csv(const std::string& input, char delimiter = '\0', bool require_table = false);

} // namespace zeus
