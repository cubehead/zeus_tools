#include "zeus/json_formatter.h"
#include "zeus/text_document.h"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    const std::size_t target_bytes = argc > 1
        ? static_cast<std::size_t>(std::stoull(argv[1]))
        : 10U * 1024U * 1024U;

    std::string input;
    input.reserve(target_bytes + 1024);
    input.push_back('[');
    std::size_t id = 0;
    while (true) {
        const std::string row = "{\"id\":" + std::to_string(id) +
            ",\"active\":true,\"name\":\"Zeus Tools benchmark row\"}";
        const std::size_t separator = id == 0 ? 0 : 1;
        if (input.size() + separator + row.size() + 1 > target_bytes) break;
        if (separator != 0) input.push_back(',');
        input += row;
        ++id;
    }
    input.push_back(']');
    input.resize(target_bytes, ' ');

    const auto format_start = std::chrono::steady_clock::now();
    const auto result = zeus::format_json(input);
    const auto format_end = std::chrono::steady_clock::now();
    if (!result.ok) {
        std::cerr << result.issue.code << ": " << result.issue.message << '\n';
        return 1;
    }

    const auto highlight_start = std::chrono::steady_clock::now();
    const auto document = zeus::HighlightedDocument::json(result.value);
    const auto highlight_end = std::chrono::steady_clock::now();

    const auto search_start = std::chrono::steady_clock::now();
    const auto matches = document.search("benchmark row", true, false);
    const auto search_end = std::chrono::steady_clock::now();

    const auto millis = [](auto from, auto to) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(to - from).count();
    };

    std::cout << "input_bytes=" << input.size() << '\n'
              << "output_bytes=" << result.value.size() << '\n'
              << "rows=" << id << '\n'
              << "formatted_lines=" << document.lines().size() << '\n'
              << "matches=" << matches.size() << '\n'
              << "format_ms=" << millis(format_start, format_end) << '\n'
              << "highlight_ms=" << millis(highlight_start, highlight_end) << '\n'
              << "search_ms=" << millis(search_start, search_end) << '\n';
    return 0;
}
