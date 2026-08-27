#include "processing_service.h"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>

namespace {

constexpr std::size_t kDefaultBytes = 10U * 1024U * 1024U;

std::string make_json(std::size_t target_bytes, std::size_t& rows) {
    std::string input;
    input.reserve(target_bytes + 1024);
    input.push_back('[');
    while (true) {
        const std::string row = "{\"id\":" + std::to_string(rows) +
            ",\"active\":true,\"name\":\"Zeus pipeline benchmark row\"}";
        const std::size_t separator = rows == 0 ? 0 : 1;
        if (input.size() + separator + row.size() + 1 > target_bytes) break;
        if (separator != 0) input.push_back(',');
        input += row;
        ++rows;
    }
    input.push_back(']');
    input.resize(target_bytes, ' ');
    return input;
}

std::string make_csv(std::size_t target_bytes, std::size_t& rows) {
    std::string input = "id,name,city,status,notes\n";
    input.reserve(target_bytes + 1024);
    while (true) {
        const std::string row = std::to_string(rows) +
            ",Zeus pipeline benchmark,Shanghai,Active,searchable CSV fixture\n";
        const std::string final_row = std::to_string(rows + 1) +
            ",Zeus pipeline benchmark,Shanghai,Active,searchable CSV fixture";
        if (input.size() + row.size() + final_row.size() > target_bytes) break;
        input += row;
        ++rows;
    }
    const std::string final_row = std::to_string(rows) +
        ",Zeus pipeline benchmark,Shanghai,Active,searchable CSV fixture";
    if (input.size() + final_row.size() > target_bytes) return {};
    input += final_row;
    input.resize(target_bytes, ' ');
    ++rows;
    return input;
}

long long millis(std::chrono::steady_clock::time_point from,
                 std::chrono::steady_clock::time_point to) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(to - from).count();
}

bool benchmark_json(std::size_t target_bytes) {
    std::size_t rows = 0;
    app::processing::AnalysisRequest request;
    request.input = make_json(target_bytes, rows);

    const auto started = std::chrono::steady_clock::now();
    const auto result = app::processing::analyze(request);
    const auto analyzed = std::chrono::steady_clock::now();
    if (!result.process.ok || result.detected != zeus::ContentKind::Json || !result.document) {
        std::cerr << "JSON pipeline did not produce a highlighted JSON document\n";
        return false;
    }
    const auto matches = result.document->search("pipeline benchmark row", true, false);
    const auto searched = std::chrono::steady_clock::now();
    if (matches.size() != rows) {
        std::cerr << "JSON pipeline search count mismatch\n";
        return false;
    }

    std::cout << "json_input_bytes=" << request.input.size() << '\n'
              << "json_output_bytes=" << result.process.value.size() << '\n'
              << "json_rows=" << rows << '\n'
              << "json_lines=" << result.document->lines().size() << '\n'
              << "json_matches=" << matches.size() << '\n'
              << "json_analyze_ms=" << millis(started, analyzed) << '\n'
              << "json_search_ms=" << millis(analyzed, searched) << '\n';
    return true;
}

bool benchmark_csv(std::size_t target_bytes) {
    std::size_t rows = 0;
    app::processing::AnalysisRequest request;
    request.input = make_csv(target_bytes, rows);
    request.input_type_id = "csv";

    const auto started = std::chrono::steady_clock::now();
    const auto result = app::processing::analyze(request);
    const auto analyzed = std::chrono::steady_clock::now();
    if (!result.process.ok || !result.csv || result.document) {
        std::cerr << "CSV pipeline did not produce one non-duplicated table document\n";
        return false;
    }
    const auto matches = result.csv->search("searchable CSV fixture", false, false);
    const auto searched = std::chrono::steady_clock::now();
    if (matches.size() != rows) {
        std::cerr << "CSV pipeline search count mismatch\n";
        return false;
    }

    std::cout << "csv_input_bytes=" << request.input.size() << '\n'
              << "csv_output_bytes=" << result.process.value.size() << '\n'
              << "csv_rows=" << result.csv->rows.size() << '\n'
              << "csv_columns=" << (result.csv->rows.empty()
                    ? 0 : result.csv->rows.front().size()) << '\n'
              << "csv_matches=" << matches.size() << '\n'
              << "csv_analyze_ms=" << millis(started, analyzed) << '\n'
              << "csv_search_ms=" << millis(analyzed, searched) << '\n';
    return true;
}

} // namespace

int main(int argc, char** argv) {
    const std::size_t target_bytes = argc > 1
        ? static_cast<std::size_t>(std::stoull(argv[1]))
        : kDefaultBytes;
    return benchmark_json(target_bytes) && benchmark_csv(target_bytes) ? 0 : 1;
}
