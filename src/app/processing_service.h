#pragma once

#include "zeus/csv_document.h"
#include "zeus/text_document.h"
#include "zeus/text_processor.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace app::processing {

inline constexpr std::size_t kRecommendedMaxInputBytes = 10U * 1024U * 1024U;
inline constexpr std::size_t kInteractiveEditorMaxBytes = 1U * 1024U * 1024U;

constexpr bool exceeds_recommended_input_size(std::size_t bytes) noexcept {
    return bytes > kRecommendedMaxInputBytes;
}

constexpr bool requires_lightweight_input_preview(std::size_t bytes) noexcept {
    return bytes > kInteractiveEditorMaxBytes;
}

struct AnalysisRequest {
    std::string input;
    std::string action_id = "auto";
    std::string input_type_id = "auto";
    int csv_delimiter_index = 0;
    bool first_row_header = true;
    bool build_presentation = true;
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
