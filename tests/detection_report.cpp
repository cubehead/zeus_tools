#include "detection_corpus.h"

#include "zeus/text_processor.h"

#include <iomanip>
#include <iostream>
#include <map>
#include <string>

namespace {

double percent(std::size_t numerator, std::size_t denominator) {
    return denominator == 0
        ? 100.0
        : 100.0 * static_cast<double>(numerator) / static_cast<double>(denominator);
}

} // namespace

int main(int argc, char** argv) {
    const std::string path = argc > 1
        ? argv[1]
        : std::string(ZEUS_TEST_FIXTURE_DIR) + "/detection-v1.tsv";
    const test_support::DetectionCorpus corpus =
        test_support::load_detection_corpus(path);
    if (!corpus.error.empty()) {
        std::cerr << "Detection corpus error: " << corpus.error << '\n';
        return 2;
    }

    std::size_t exact = 0;
    std::size_t recognizable = 0;
    std::size_t recognizable_correct = 0;
    std::size_t plain = 0;
    std::size_t plain_correct = 0;
    std::size_t error_cases = 0;
    std::size_t errors_preserved = 0;
    std::map<std::string, std::size_t> coverage;

    for (const auto& item : corpus.cases) {
        const zeus::ProcessResult result = zeus::process_text(item.input);
        const std::string actual_kind = zeus::content_kind_name(result.detected);
        const bool kind_matches = actual_kind == item.expected_kind;
        const bool exact_match = kind_matches && result.ok == item.expected_ok;
        exact += exact_match ? 1 : 0;
        ++coverage[item.expected_kind];

        if (item.expected_kind == "Text") {
            ++plain;
            plain_correct += kind_matches ? 1 : 0;
        } else if (item.expected_kind != "Empty") {
            ++recognizable;
            recognizable_correct += kind_matches ? 1 : 0;
        }
        if (!item.expected_ok) {
            ++error_cases;
            errors_preserved += exact_match ? 1 : 0;
        }
        if (!exact_match) {
            std::cerr << item.name << ": expected " << item.expected_kind
                      << " (ok=" << item.expected_ok << "), got " << actual_kind
                      << " (ok=" << result.ok << ")\n";
        }
    }

    std::cout << std::fixed << std::setprecision(2)
              << "corpus=detection-v1\n"
              << "cases=" << corpus.cases.size() << '\n'
              << "exact_matches=" << exact << '\n'
              << "exact_accuracy_pct=" << percent(exact, corpus.cases.size()) << '\n'
              << "recognizable_cases=" << recognizable << '\n'
              << "recognizable_recall_pct="
              << percent(recognizable_correct, recognizable) << '\n'
              << "plain_text_cases=" << plain << '\n'
              << "plain_text_specificity_pct=" << percent(plain_correct, plain) << '\n'
              << "invalid_cases=" << error_cases << '\n'
              << "invalid_preserved_pct=" << percent(errors_preserved, error_cases) << '\n'
              << "coverage=";
    bool first = true;
    for (const auto& [kind, count] : coverage) {
        if (!first) std::cout << ',';
        std::cout << kind << ':' << count;
        first = false;
    }
    std::cout << '\n';

    const bool coverage_floor = corpus.cases.size() >= 50 && recognizable >= 25 &&
        plain >= 20 && error_cases >= 1;
    if (!coverage_floor) {
        std::cerr << "Detection corpus coverage floor is not met\n";
    }
    return exact == corpus.cases.size() && coverage_floor ? 0 : 1;
}
