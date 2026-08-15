#pragma once

#include "zeus/csv_document.h"
#include "zeus/text_document.h"
#include "zeus/text_processor.h"

#include <cstdint>
#include <memory>
#include <string>

namespace app::processing {

struct AnalysisRequest {
    std::string input;
    std::string action_id = "auto";
    std::string input_type_id = "auto";
    int csv_delimiter_index = 0;
    bool first_row_header = true;
};

struct AnalysisResult {
    zeus::ProcessResult process;
    zeus::ContentKind detected = zeus::ContentKind::Text;
    std::shared_ptr<zeus::HighlightedDocument> document;
    std::shared_ptr<zeus::CsvDocument> csv;
    bool first_row_header = true;
    bool can_continue_decode = false;
    std::string decode_chain;
    std::int64_t elapsed_ms = 0;
};

struct DecodeResult {
    zeus::ProcessResult process;
    std::shared_ptr<zeus::HighlightedDocument> document;
    std::string source;
    std::string chain;
    bool can_continue = false;
};

AnalysisResult analyze(const AnalysisRequest& request);
DecodeResult decode_one_layer(std::string source, const std::string& previous_chain);

} // namespace app::processing
