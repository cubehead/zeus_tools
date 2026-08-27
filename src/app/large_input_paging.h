#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace app::large_input {

inline constexpr std::size_t kPageBytes = 16U * 1024U;
inline constexpr std::size_t kMaxInteractivePageBytes = 64U * 1024U;

struct PageRange {
    std::size_t start = 0;
    std::size_t end = 0;
};

constexpr std::size_t page_offset(std::size_t offset, const PageRange& range) noexcept {
    return std::clamp(offset, range.start, range.end) - range.start;
}

inline std::optional<std::string> replacement_text(
    std::string_view old_page,
    std::string_view new_page,
    std::size_t selection_start,
    std::size_t selection_end) {
    if (selection_start > selection_end || selection_end > old_page.size()) {
        return std::nullopt;
    }
    const std::size_t retained = old_page.size() - (selection_end - selection_start);
    if (new_page.size() < retained) return std::nullopt;
    const std::size_t inserted_size = new_page.size() - retained;
    if (new_page.substr(0, selection_start) != old_page.substr(0, selection_start) ||
        new_page.substr(selection_start + inserted_size) != old_page.substr(selection_end)) {
        return std::nullopt;
    }
    return std::string(new_page.substr(selection_start, inserted_size));
}

constexpr std::size_t page_count(std::size_t bytes) noexcept {
    return bytes == 0 ? 1 : (bytes + kPageBytes - 1) / kPageBytes;
}

inline std::size_t utf8_boundary_at(const std::string& value, std::size_t offset) {
    std::size_t boundary = std::min(offset, value.size());
    while (boundary > 0 && boundary < value.size() &&
           (static_cast<unsigned char>(value[boundary]) & 0xC0U) == 0x80U) {
        --boundary;
    }
    return boundary;
}

inline PageRange page_range(const std::string& value, std::size_t page) {
    const std::size_t count = page_count(value.size());
    const std::size_t current = std::min(page, count - 1);
    return {
        utf8_boundary_at(value, current * kPageBytes),
        current + 1 >= count
            ? value.size()
            : utf8_boundary_at(value, (current + 1) * kPageBytes),
    };
}

inline std::vector<std::size_t> page_boundaries(const std::string& value) {
    const std::size_t count = page_count(value.size());
    std::vector<std::size_t> boundaries;
    boundaries.reserve(count + 1);
    for (std::size_t page = 0; page < count; ++page) {
        boundaries.push_back(utf8_boundary_at(value, page * kPageBytes));
    }
    boundaries.push_back(value.size());
    return boundaries;
}

inline void resize_page(
    std::vector<std::size_t>& boundaries,
    std::size_t page,
    std::size_t new_size) {
    if (page + 1 >= boundaries.size()) return;
    const std::size_t old_size = boundaries[page + 1] - boundaries[page];
    for (std::size_t index = page + 1; index < boundaries.size(); ++index) {
        if (new_size >= old_size) {
            boundaries[index] += new_size - old_size;
        } else {
            boundaries[index] -= old_size - new_size;
        }
    }
}

} // namespace app::large_input
