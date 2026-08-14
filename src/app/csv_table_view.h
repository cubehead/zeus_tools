#pragma once

#include "font_tokens.h"

#include "zeus/csv_document.h"

#include "components/components.h"
#include "core/platform/platform.h"
#include "eui/dsl.h"
#include "eui/types.h"
#include "core/render/text.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>

namespace zeus::app_components {

class CsvTableViewBuilder {
public:
    CsvTableViewBuilder(eui::Ui& ui, std::string id) : ui_(ui), id_(std::move(id)) {}

    CsvTableViewBuilder& position(float x, float y) { x_ = x; y_ = y; return *this; }
    CsvTableViewBuilder& size(float width, float height) { width_ = width; height_ = height; return *this; }
    CsvTableViewBuilder& document(std::shared_ptr<const CsvDocument> value) {
        document_ = std::move(value); return *this;
    }
    CsvTableViewBuilder& scroll(float& vertical, float& horizontal) {
        vertical_scroll_ = &vertical; horizontal_scroll_ = &horizontal; return *this;
    }
    CsvTableViewBuilder& selection(std::size_t& row, std::size_t& column) {
        selected_row_ = &row; selected_column_ = &column; return *this;
    }
    CsvTableViewBuilder& firstRowHeader(bool value) {
        first_row_header_ = value; return *this;
    }
    CsvTableViewBuilder& theme(const components::theme::ThemeColorTokens& value) {
        tokens_ = value; return *this;
    }
    CsvTableViewBuilder& searchMatches(
        const std::vector<CsvSearchMatch>& value,
        std::size_t active_match) {
        search_matches_ = &value;
        active_match_ = active_match;
        return *this;
    }

    void build() {
        if (!document_ || document_->rows.empty() || !vertical_scroll_ || !horizontal_scroll_ ||
            !selected_row_ || !selected_column_) return;

        constexpr float header_height = 32.0f;
        constexpr float row_height = 28.0f;
        constexpr float row_number_width = 48.0f;
        constexpr float column_width = 180.0f;
        constexpr float scrollbar_size = 10.0f;
        const auto document = document_;
        std::size_t column_count = 1;
        for (const auto& row : document->rows) column_count = std::max(column_count, row.size());
        const std::size_t header_rows = first_row_header_ ? 1 : 0;
        const std::size_t body_rows = document->rows.size() - header_rows;
        const float body_width = std::max(1.0f, width_ - row_number_width - scrollbar_size);
        const float body_height = std::max(1.0f, height_ - header_height - scrollbar_size);
        const float content_width = static_cast<float>(column_count) * column_width;
        const float content_height = static_cast<float>(body_rows) * row_height;
        const float max_horizontal = std::max(0.0f, content_width - body_width);
        const float max_vertical = std::max(0.0f, content_height - body_height);
        *vertical_scroll_ = std::clamp(*vertical_scroll_, 0.0f, max_vertical);
        *horizontal_scroll_ = std::clamp(*horizontal_scroll_, 0.0f, max_horizontal);

        const std::size_t first_body = body_rows == 0 ? 0
            : std::min(body_rows - 1, static_cast<std::size_t>(*vertical_scroll_ / row_height));
        const float row_offset = body_rows == 0 ? 0.0f
            : static_cast<float>(first_body) * row_height - *vertical_scroll_;
        const std::size_t visible_rows = static_cast<std::size_t>(std::ceil(body_height / row_height)) + 1;
        const std::size_t last_body = std::min(body_rows, first_body + visible_rows);
        const std::size_t first_column = std::min(
            column_count - 1, static_cast<std::size_t>(*horizontal_scroll_ / column_width));
        const std::size_t visible_columns = static_cast<std::size_t>(std::ceil(body_width / column_width)) + 1;
        const std::size_t last_column = std::min(column_count, first_column + visible_columns);

        const auto background = tokens_.dark
            ? eui::Color{0.075f, 0.086f, 0.106f, 1.0f}
            : eui::Color{0.985f, 0.988f, 0.995f, 1.0f};
        const auto header = tokens_.dark
            ? eui::Color{0.105f, 0.120f, 0.148f, 1.0f}
            : eui::Color{0.925f, 0.942f, 0.970f, 1.0f};
        const auto alternate = tokens_.dark
            ? eui::Color{0.090f, 0.102f, 0.125f, 1.0f}
            : eui::Color{0.966f, 0.974f, 0.989f, 1.0f};
        const auto selected = tokens_.dark
            ? eui::Color{0.16f, 0.34f, 0.54f, 0.92f}
            : eui::Color{0.70f, 0.84f, 1.0f, 0.92f};
        const auto matched = tokens_.dark
            ? eui::Color{0.42f, 0.31f, 0.10f, 0.92f}
            : eui::Color{1.0f, 0.88f, 0.48f, 0.82f};
        const auto current = tokens_.dark
            ? eui::Color{0.54f, 0.25f, 0.10f, 0.96f}
            : eui::Color{1.0f, 0.70f, 0.30f, 0.90f};

        ui_.stack(id_)
            .position(x_, y_)
            .size(width_, height_)
            .clip()
            .content([&] {
                ui_.rect(id_ + ".background").size(width_, height_)
                    .color(background).border(1.0f, tokens_.border).radius(6.0f).build();

                ui_.stack(id_ + ".body")
                    .position(row_number_width, header_height)
                    .size(body_width, body_height)
                    .clip()
                    .content([&] {
                        for (std::size_t visible = first_body; visible < last_body; ++visible) {
                            const std::size_t document_row = visible + header_rows;
                            const float y = row_offset + static_cast<float>(visible - first_body) * row_height;
                            ui_.rect(id_ + ".row." + std::to_string(document_row))
                                .position(0.0f, y).size(body_width, row_height)
                                .color(document_row % 2 == 0 ? background : alternate).build();
                            for (std::size_t column = first_column; column < last_column; ++column) {
                                const float x = static_cast<float>(column) * column_width - *horizontal_scroll_;
                                const bool is_selected = *selected_row_ == document_row && *selected_column_ == column;
                                if (is_selected) {
                                    ui_.rect(id_ + ".cell.bg." + std::to_string(document_row) + "." + std::to_string(column))
                                        .position(x, y).size(column_width, row_height)
                                        .color(selected).build();
                                }
                                const std::string value = column < document->rows[document_row].size()
                                    ? document->rows[document_row][column] : std::string{};
                                const CellPreview preview = makeCellPreview(value);
                                drawSearchMatches(
                                    document_row, column, preview, x + 8.0f, y + 2.0f,
                                    row_height - 4.0f, matched, current);
                                ui_.text(id_ + ".cell." + std::to_string(document_row) + "." + std::to_string(column))
                                    .position(x + 8.0f, y).size(column_width - 16.0f, row_height)
                                    .text(preview.text).fontSize(13.0f).fontFamily(app::fonts::code())
                                    .verticalAlign(eui::VerticalAlign::Center).color(tokens_.text).build();
                                ui_.rect(id_ + ".cell.divider." + std::to_string(document_row) + "." + std::to_string(column))
                                    .position(x + column_width - 1.0f, y).size(1.0f, row_height)
                                    .color(tokens_.border).build();
                            }
                            ui_.rect(id_ + ".row.divider." + std::to_string(document_row))
                                .position(0.0f, y + row_height - 1.0f).size(body_width, 1.0f)
                                .color(tokens_.border).build();
                        }
                    }).build();

                ui_.rect(id_ + ".header.row_number").size(row_number_width, header_height)
                    .color(header).build();
                ui_.text(id_ + ".header.row_number.text").size(row_number_width, header_height)
                    .text("#").fontFamily(app::fonts::code()).fontSize(12.5f).fontWeight(700)
                    .horizontalAlign(eui::HorizontalAlign::Center)
                    .verticalAlign(eui::VerticalAlign::Center)
                    .color(tokens_.dark ? eui::Color{0.62f, 0.67f, 0.75f, 1.0f}
                                        : eui::Color{0.38f, 0.43f, 0.50f, 1.0f}).build();

                ui_.stack(id_ + ".header")
                    .position(row_number_width, 0.0f).size(body_width, header_height).clip()
                    .content([&] {
                        ui_.rect(id_ + ".header.background").size(body_width, header_height).color(header).build();
                        for (std::size_t column = first_column; column < last_column; ++column) {
                            const float x = static_cast<float>(column) * column_width - *horizontal_scroll_;
                            const bool is_selected = first_row_header_ &&
                                *selected_row_ == 0 && *selected_column_ == column;
                            if (is_selected) {
                                ui_.rect(id_ + ".header.cell.bg." + std::to_string(column))
                                    .position(x, 0.0f).size(column_width, header_height)
                                    .color(selected).build();
                            }
                            const std::string value = first_row_header_ &&
                                    column < document->rows.front().size()
                                ? document->rows.front()[column]
                                : "Column " + std::to_string(column + 1);
                            const CellPreview preview = makeCellPreview(value);
                            if (first_row_header_) {
                                drawSearchMatches(
                                    0, column, preview, x + 8.0f, 2.0f,
                                    header_height - 4.0f, matched, current);
                            }
                            ui_.text(id_ + ".header.cell." + std::to_string(column))
                                .position(x + 8.0f, 0.0f).size(column_width - 16.0f, header_height)
                                .text(preview.text).fontSize(13.0f).fontWeight(700).fontFamily(app::fonts::code())
                                .verticalAlign(eui::VerticalAlign::Center).color(tokens_.text).build();
                            ui_.rect(id_ + ".header.divider." + std::to_string(column))
                                .position(x + column_width - 1.0f, 0.0f).size(1.0f, header_height)
                                .color(tokens_.border).build();
                        }
                    }).build();

                ui_.stack(id_ + ".row_numbers")
                    .position(0.0f, header_height).size(row_number_width, body_height).clip()
                    .content([&] {
                        for (std::size_t visible = first_body; visible < last_body; ++visible) {
                            const std::size_t document_row = visible + header_rows;
                            const float y = row_offset + static_cast<float>(visible - first_body) * row_height;
                            ui_.rect(id_ + ".row_number.bg." + std::to_string(document_row))
                                .position(0.0f, y).size(row_number_width, row_height)
                                .color(header).build();
                            ui_.text(id_ + ".row_number." + std::to_string(document_row))
                                .position(0.0f, y).size(row_number_width - 7.0f, row_height)
                                .text(std::to_string(visible + 1)).fontFamily(app::fonts::code()).fontSize(12.0f)
                                .horizontalAlign(eui::HorizontalAlign::Right)
                                .verticalAlign(eui::VerticalAlign::Center)
                                .color(tokens_.dark ? eui::Color{0.62f, 0.67f, 0.75f, 1.0f}
                                                    : eui::Color{0.38f, 0.43f, 0.50f, 1.0f}).build();
                        }
                    }).build();

                const float hit_height = header_height + body_height;
                ui_.rect(id_ + ".interaction")
                    .position(row_number_width, 0.0f).size(body_width, hit_height)
                    .zIndex(10).color({0.0f, 0.0f, 0.0f, 0.0f}).interactive().focusable()
                    .cursor(core::CursorShape::Arrow)
                    .onScroll([this, max_vertical, max_horizontal](const core::ScrollEvent& event) {
                        const float step_x = std::clamp(static_cast<float>(event.x), -4.0f, 4.0f) * 42.0f;
                        const float step_y = std::clamp(static_cast<float>(event.y), -4.0f, 4.0f) * 42.0f;
                        if (std::fabs(step_x) > std::fabs(step_y)) {
                            *horizontal_scroll_ = std::clamp(*horizontal_scroll_ - step_x, 0.0f, max_horizontal);
                        } else {
                            *vertical_scroll_ = std::clamp(*vertical_scroll_ - step_y, 0.0f, max_vertical);
                        }
                        core::platform::requestUiUpdate();
                    })
                    .onPress([this, document, header_height, row_height, column_width,
                              header_rows, body_rows, column_count, body_width, hit_height](
                                 const core::PointerEvent& event, const core::Rect& bounds) {
                        const float scale_x = bounds.width > 0.0f ? bounds.width / body_width : 1.0f;
                        const float scale_y = bounds.height > 0.0f ? bounds.height / hit_height : 1.0f;
                        const float local_x = static_cast<float>(event.x - bounds.x) / std::max(0.001f, scale_x);
                        const float local_y = static_cast<float>(event.y - bounds.y) / std::max(0.001f, scale_y);
                        const std::size_t column = static_cast<std::size_t>(std::max(
                            0.0f, std::floor((local_x + *horizontal_scroll_) / column_width)));
                        if (local_y < header_height && !first_row_header_) return;
                        const std::size_t row = local_y < header_height ? 0
                            : header_rows + static_cast<std::size_t>(std::max(0.0f,
                                std::floor((local_y - header_height + *vertical_scroll_) / row_height)));
                        if (row < document->rows.size() && column < column_count) {
                            *selected_row_ = row;
                            *selected_column_ = column;
                            core::platform::requestUiUpdate();
                        }
                    }).build();

                if (max_vertical > 0.0f) {
                    const float thumb_height = std::max(22.0f, body_height * body_height / content_height);
                    const float travel = std::max(0.0f, body_height - thumb_height);
                    const float thumb_y = header_height + (*vertical_scroll_ / max_vertical) * travel;
                    ui_.rect(id_ + ".vscroll.track").position(width_ - scrollbar_size, header_height)
                        .size(scrollbar_size, body_height).color(header).build();
                    ui_.rect(id_ + ".vscroll.thumb").position(width_ - scrollbar_size + 2.0f, thumb_y)
                        .size(scrollbar_size - 4.0f, thumb_height).color(tokens_.primary).radius(3.0f).build();
                    ui_.rect(id_ + ".vscroll.hit").position(width_ - scrollbar_size, header_height)
                        .size(scrollbar_size, body_height).zIndex(20)
                        .color({0.0f, 0.0f, 0.0f, 0.0f}).interactive()
                        .onPress([this, max_vertical, body_height, thumb_height](
                                     const core::PointerEvent& event, const core::Rect& bounds) {
                            const float scale = bounds.height > 0.0f ? bounds.height / body_height : 1.0f;
                            const float local = static_cast<float>(event.y - bounds.y) / std::max(0.001f, scale);
                            const float travel = std::max(1.0f, body_height - thumb_height);
                            const float ratio = std::clamp((local - thumb_height * 0.5f) / travel, 0.0f, 1.0f);
                            *vertical_scroll_ = ratio * max_vertical;
                            core::platform::requestUiUpdate();
                        }).build();
                }
                if (max_horizontal > 0.0f) {
                    const float thumb_width = std::max(30.0f, body_width * body_width / content_width);
                    const float travel = std::max(0.0f, body_width - thumb_width);
                    const float thumb_x = row_number_width + (*horizontal_scroll_ / max_horizontal) * travel;
                    ui_.rect(id_ + ".hscroll.track").position(row_number_width, height_ - scrollbar_size)
                        .size(body_width, scrollbar_size).color(header).build();
                    ui_.rect(id_ + ".hscroll.thumb").position(thumb_x, height_ - scrollbar_size + 2.0f)
                        .size(thumb_width, scrollbar_size - 4.0f).color(tokens_.primary).radius(3.0f).build();
                    ui_.rect(id_ + ".hscroll.hit").position(row_number_width, height_ - scrollbar_size)
                        .size(body_width, scrollbar_size).zIndex(20)
                        .color({0.0f, 0.0f, 0.0f, 0.0f}).interactive()
                        .onPress([this, max_horizontal, body_width, thumb_width](
                                     const core::PointerEvent& event, const core::Rect& bounds) {
                            const float scale = bounds.width > 0.0f ? bounds.width / body_width : 1.0f;
                            const float local = static_cast<float>(event.x - bounds.x) / std::max(0.001f, scale);
                            const float travel = std::max(1.0f, body_width - thumb_width);
                            const float ratio = std::clamp((local - thumb_width * 0.5f) / travel, 0.0f, 1.0f);
                            *horizontal_scroll_ = ratio * max_horizontal;
                            core::platform::requestUiUpdate();
                        }).build();
                }
            }).build();
    }

private:
    struct CellPreview {
        std::string text;
        std::vector<std::size_t> source_to_preview;
        std::size_t source_limit = 0;
    };

    static CellPreview makeCellPreview(const std::string& value) {
        constexpr std::size_t max_bytes = 240;
        std::size_t source_limit = std::min(value.size(), max_bytes);
        while (source_limit > 0 && source_limit < value.size() &&
               (static_cast<unsigned char>(value[source_limit]) & 0xC0U) == 0x80U) {
            --source_limit;
        }
        CellPreview preview;
        preview.source_limit = source_limit;
        preview.text.reserve(source_limit + 8);
        preview.source_to_preview.resize(source_limit + 1);
        for (std::size_t i = 0; i < source_limit; ++i) {
            preview.source_to_preview[i] = preview.text.size();
            const char ch = value[i];
            if (ch == '\n' || ch == '\r') preview.text += " ↵ ";
            else if (ch == '\t') preview.text += "  ";
            else if (static_cast<unsigned char>(ch) < 0x20U) preview.text.push_back(' ');
            else preview.text.push_back(ch);
        }
        preview.source_to_preview[source_limit] = preview.text.size();
        if (source_limit < value.size()) preview.text += "…";
        return preview;
    }

    static float caretX(
        const core::TextPrimitive::TextMetrics& metrics,
        std::size_t byte_offset) {
        if (metrics.byteIndices.empty() || metrics.caretX.empty()) return 0.0f;
        const auto it = std::lower_bound(
            metrics.byteIndices.begin(), metrics.byteIndices.end(),
            static_cast<int>(byte_offset));
        const std::size_t index = it == metrics.byteIndices.end()
            ? metrics.caretX.size() - 1
            : static_cast<std::size_t>(std::distance(metrics.byteIndices.begin(), it));
        return metrics.caretX[std::min(index, metrics.caretX.size() - 1)];
    }

    void drawSearchMatches(
        std::size_t row,
        std::size_t column,
        const CellPreview& preview,
        float x,
        float y,
        float height,
        const eui::Color& matched,
        const eui::Color& current) {
        if (search_matches_ == nullptr || search_matches_->empty()) return;
        const auto first = std::lower_bound(
            search_matches_->begin(), search_matches_->end(), std::pair{row, column},
            [](const CsvSearchMatch& match, const std::pair<std::size_t, std::size_t>& cell) {
                return match.row < cell.first ||
                    (match.row == cell.first && match.column < cell.second);
            });
        if (first == search_matches_->end() || first->row != row || first->column != column) return;
        const auto metrics = core::TextPrimitive::measureTextMetrics(
            preview.text, app::fonts::code(), 13.0f, 400);
        for (auto it = first;
             it != search_matches_->end() && it->row == row && it->column == column;
             ++it) {
            if (it->start >= preview.source_limit || it->length == 0) continue;
            const std::size_t source_end = std::min(
                preview.source_limit, it->start + it->length);
            const std::size_t preview_start = preview.source_to_preview[it->start];
            const std::size_t preview_end = preview.source_to_preview[source_end];
            const float start_x = caretX(metrics, preview_start);
            const float end_x = caretX(metrics, preview_end);
            const std::size_t match_index = static_cast<std::size_t>(
                std::distance(search_matches_->begin(), it));
            ui_.rect(id_ + ".search." + std::to_string(match_index))
                .position(x + start_x, y)
                .size(std::max(2.0f, end_x - start_x), height)
                .color(match_index == active_match_ ? current : matched)
                .radius(3.0f)
                .build();
        }
    }

    eui::Ui& ui_;
    std::string id_;
    std::shared_ptr<const CsvDocument> document_;
    float* vertical_scroll_ = nullptr;
    float* horizontal_scroll_ = nullptr;
    std::size_t* selected_row_ = nullptr;
    std::size_t* selected_column_ = nullptr;
    components::theme::ThemeColorTokens tokens_ = components::theme::dark();
    const std::vector<CsvSearchMatch>* search_matches_ = nullptr;
    std::size_t active_match_ = 0;
    bool first_row_header_ = true;
    float x_ = 0.0f;
    float y_ = 0.0f;
    float width_ = 320.0f;
    float height_ = 220.0f;
};

inline CsvTableViewBuilder csvTableView(eui::Ui& ui, const std::string& id) {
    return CsvTableViewBuilder(ui, id);
}

} // namespace zeus::app_components
