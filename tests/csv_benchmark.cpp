#include "zeus/csv_document.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    const std::size_t target_bytes = argc > 1
        ? static_cast<std::size_t>(std::stoull(argv[1]))
        : 10U * 1024U * 1024U;

    std::string input = "id,name,city,email,role,status,notes\n";
    input.reserve(target_bytes + 1024);
    std::size_t rows = 0;
    while (input.size() < target_bytes) {
        input += std::to_string(rows) +
            ",Zeus benchmark row,Shanghai,benchmark@example.com,Engineer,Active,"
            "searchable CSV performance fixture\n";
        ++rows;
    }

    const auto parse_start = std::chrono::steady_clock::now();
    const auto parsed = zeus::parse_csv(input, '\0', true);
    const auto parse_end = std::chrono::steady_clock::now();
    if (!parsed.ok) {
        std::cerr << parsed.error << '\n';
        return 1;
    }

    const std::string query = "performance fixture";
    std::size_t matches = 0;
    const auto search_start = std::chrono::steady_clock::now();
    const auto equal_ascii_ci = [](unsigned char left, unsigned char right) {
        return std::tolower(left) == std::tolower(right);
    };
    for (const auto& row : parsed.document.rows) {
        for (const auto& cell : row) {
            if (std::search(cell.begin(), cell.end(), query.begin(), query.end(), equal_ascii_ci) != cell.end()) {
                ++matches;
            }
        }
    }
    const auto search_end = std::chrono::steady_clock::now();

    const auto millis = [](auto from, auto to) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(to - from).count();
    };
    std::cout << "input_bytes=" << input.size() << '\n'
              << "rows=" << parsed.document.rows.size() << '\n'
              << "columns=" << parsed.document.rows.front().size() << '\n'
              << "matches=" << matches << '\n'
              << "parse_ms=" << millis(parse_start, parse_end) << '\n'
              << "search_ms=" << millis(search_start, search_end) << '\n';
    return 0;
}
