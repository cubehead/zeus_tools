#include "detection_corpus.h"

#include <fstream>
#include <set>
#include <utility>

namespace test_support {

namespace {

std::string unescape_fixture(const std::string& value) {
    std::string output;
    output.reserve(value.size());
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (value[index] != '\\' || index + 1 >= value.size()) {
            output.push_back(value[index]);
            continue;
        }
        const char escaped = value[++index];
        switch (escaped) {
        case 'n': output.push_back('\n'); break;
        case 'r': output.push_back('\r'); break;
        case 't': output.push_back('\t'); break;
        case '\\': output.push_back('\\'); break;
        default:
            output.push_back('\\');
            output.push_back(escaped);
            break;
        }
    }
    return output;
}

} // namespace

DetectionCorpus load_detection_corpus(const std::string& path) {
    DetectionCorpus corpus;
    std::ifstream input(path);
    if (!input) {
        corpus.error = "cannot open detection corpus: " + path;
        return corpus;
    }

    std::set<std::string> names;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (line.empty() || line.front() == '#') continue;
        const std::size_t first = line.find('\t');
        const std::size_t second = first == std::string::npos
            ? std::string::npos : line.find('\t', first + 1);
        const std::size_t third = second == std::string::npos
            ? std::string::npos : line.find('\t', second + 1);
        if (first == std::string::npos || second == std::string::npos ||
            third == std::string::npos) {
            corpus.error = "malformed row at line " + std::to_string(line_number);
            corpus.cases.clear();
            return corpus;
        }

        DetectionCase item;
        item.name = line.substr(0, first);
        item.expected_kind = line.substr(first + 1, second - first - 1);
        const std::string expected_ok = line.substr(second + 1, third - second - 1);
        if (item.name.empty() || item.expected_kind.empty() ||
            (expected_ok != "true" && expected_ok != "false")) {
            corpus.error = "invalid fields at line " + std::to_string(line_number);
            corpus.cases.clear();
            return corpus;
        }
        if (!names.insert(item.name).second) {
            corpus.error = "duplicate case name at line " + std::to_string(line_number) +
                ": " + item.name;
            corpus.cases.clear();
            return corpus;
        }
        item.expected_ok = expected_ok == "true";
        item.input = unescape_fixture(line.substr(third + 1));
        corpus.cases.push_back(std::move(item));
    }
    if (corpus.cases.empty()) corpus.error = "detection corpus is empty";
    return corpus;
}

} // namespace test_support
