#pragma once

#include <string>
#include <vector>

namespace test_support {

struct DetectionCase {
    std::string name;
    std::string expected_kind;
    bool expected_ok = true;
    std::string input;
};

struct DetectionCorpus {
    std::vector<DetectionCase> cases;
    std::string error;
};

DetectionCorpus load_detection_corpus(const std::string& path);

} // namespace test_support
