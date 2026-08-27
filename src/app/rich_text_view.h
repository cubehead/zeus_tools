#pragma once

#include "font_tokens.h"
#include "dynamic_virtual_list.h"

#include "components/components.h"
#include "core/platform/platform.h"
#include "eui/dsl.h"
#include "eui/types.h"

#include "zeus/text_document.h"
#include "zeus/text_selection.h"
#include "core/render/text.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace zeus::app_components {

class RichTextViewBuilder {
public:
    RichTextViewBuilder(eui::Ui& ui, std::string id)
        : ui_(ui), id_(std::move(id)) {}

    RichTextViewBuilder& position(float x, float y) {
        x_ = x;
        y_ = y;
        return *this;
    }

    RichTextViewBuilder& size(float width, float height) {
        width_ = std::max(0.0f, width);
        height_ = std::max(0.0f, height);
        return *this;
    }

    RichTextViewBuilder& document(std::shared_ptr<const HighlightedDocument> value) {
        document_ = std::move(value);
        return *this;
    }

    RichTextViewBuilder& selection(TextSelection& value) {
        selection_ = &value;
        return *this;
    }

    RichTextViewBuilder& scroll(float& value) {
        scroll_ = &value;
        return *this;
    }

    RichTextViewBuilder& folds(TextFoldState& value) {
        folds_ = &value;
        return *this;
    }

    RichTextViewBuilder& rowHeight(float value) {
        row_height_ = std::max(1.0f, value);
        return *this;
    }

    RichTextViewBuilder& dark(bool value) {
        dark_ = value;
        return *this;
    }

    RichTextViewBuilder& theme(const ::components::theme::ThemeColorTokens& value) {
        tokens_ = value;
        return *this;
    }

    RichTextViewBuilder& searchMatches(
        const std::vector<SearchMatch>& value,
        std::size_t active_match) {
        search_matches_ = &value;
        active_match_ = active_match;
        return *this;
    }

    RichTextViewBuilder& onCopy(std::function<void()> value) {
        on_copy_ = std::move(value);
        return *this;
    }

    void build() {
        if (!document_ || !selection_ || !scroll_ || document_->lines().empty()) return;

        struct InteractionState {
            float origin_x = 0.0f;
            float origin_y = 0.0f;
            float scale = 1.0f;
            float last_press_x = 0.0f;
            float last_press_y = 0.0f;
            std::chrono::steady_clock::time_point last_press_time{};
            int click_count = 0;
        };

        InteractionState& interaction = ui_.state<InteractionState>(id_ + ".interaction");
        const auto document = document_;
        TextSelection* selection = selection_;
        TextFoldState* folds = folds_;
        float* scroll = scroll_;
        const float row_height = row_height_;
        const float viewport_width = width_;
        const float viewport_height = height_;
        const bool dark = dark_;
        const auto tokens = tokens_;
        const auto* search_matches = search_matches_;
        const std::size_t active_match = active_match_;
        const auto on_copy = on_copy_;
        if (folds != nullptr) folds->ensure_document(*document);
        const std::string fold_generation = folds != nullptr
            ? std::to_string(folds->revision())
            : std::string{};
        const auto visible_lines = std::make_shared<const std::vector<std::size_t>>(
            folds != nullptr ? folds->visible_lines(*document) : allLines(*document));

        dynamicVirtualList(ui_, id_ + ".list")
            .position(x_, y_)
            .size(viewport_width, viewport_height)
            .itemCount(static_cast<std::int64_t>(visible_lines->size()))
            .rowHeight(row_height)
            .offset(*scroll)
            .step(row_height * 3.0f)
            .overscanViewports(0.75f)
            .scrollbarWidth(7.0f)
            .scrollbarGap(8.0f)
            .theme(tokens)
            .transition(eui::Transition{})
            .contentKey(fold_generation)
            .onChange([scroll](float value) { *scroll = value; })
            .row([document, selection, folds, visible_lines, dark, search_matches, active_match](
                     eui::Ui& row_ui,
                     const std::string& row_id,
                     std::int64_t index,
                     float width,
                     float height) {
                const auto line_index = (*visible_lines)[static_cast<std::size_t>(index)];
                const auto& line = document->lines()[line_index];
                const FoldRegion* fold_region = document->fold_region_at(line_index);
                const bool collapsed = folds != nullptr && folds->is_collapsed(line_index);
                const std::size_t rendered_length = std::min(
                    line.text.size(), max_rendered_line_bytes_);
                const std::string rendered_text = line.text.substr(0, rendered_length);
                const auto metrics = core::TextPrimitive::measureTextMetrics(
                    rendered_text, app::fonts::code(), 14.0f, 400);
                if (search_matches != nullptr && !search_matches->empty()) {
                    const auto first = std::lower_bound(
                        search_matches->begin(), search_matches->end(), line_index,
                        [](const SearchMatch& match, std::size_t target_line) {
                            return match.line < target_line;
                        });
                    for (auto it = first;
                         it != search_matches->end() && it->line == line_index;
                         ++it) {
                        const std::size_t match_index = static_cast<std::size_t>(
                            std::distance(search_matches->begin(), it));
                        const std::size_t start = std::min(it->column, rendered_length);
                        const std::size_t end = std::min(
                            rendered_length, start + it->length);
                        if (start >= end) continue;
                        const float match_start_x = caretX(metrics, start);
                        const float match_end_x = caretX(metrics, end);
                        const bool current = match_index == active_match;
                        row_ui.rect(row_id + ".match." + std::to_string(match_index))
                            .position(text_x_ + match_start_x, 2.0f)
                            .size(std::max(2.0f, match_end_x - match_start_x), height - 4.0f)
                            .color(dark
                                ? (current ? eui::Color{0.94f, 0.55f, 0.08f, 0.68f}
                                           : eui::Color{0.72f, 0.58f, 0.08f, 0.34f})
                                : (current ? eui::Color{1.00f, 0.62f, 0.04f, 0.58f}
                                           : eui::Color{1.00f, 0.84f, 0.22f, 0.38f}))
                            .radius(3.0f)
                            .build();
                    }
                }

                auto columns = selection->columns_for_line(*document, line_index);
                columns.first = std::min(columns.first, rendered_length);
                columns.second = std::min(columns.second, rendered_length);
                const bool newline = selection->includes_line_break_after(*document, line_index);
                if (columns.first < columns.second || newline) {
                    const float selected_start_x = caretX(metrics, columns.first);
                    const float selected_end_x = caretX(metrics, columns.second);
                    row_ui.rect(row_id + ".selection")
                        .position(text_x_ + selected_start_x, 1.0f)
                        .size(std::max(
                                  2.0f,
                                  selected_end_x - selected_start_x +
                                      (newline ? 3.0f : 0.0f)),
                              height - 2.0f)
                        .color(dark ? eui::Color{0.18f, 0.46f, 0.82f, 0.38f}
                                    : eui::Color{0.16f, 0.42f, 0.82f, 0.25f})
                        .radius(2.0f)
                        .build();
                }

                row_ui.text(row_id + ".line")
                    .size(49.0f, height)
                    .text(std::to_string(line_index + 1))
                    .fontFamily(app::fonts::code())
                    .fontSize(13.0f)
                    .horizontalAlign(eui::HorizontalAlign::Right)
                    .verticalAlign(eui::VerticalAlign::Center)
                    .color(dark ? eui::Color{0.40f, 0.45f, 0.54f, 1.0f}
                                : eui::Color{0.47f, 0.50f, 0.57f, 1.0f})
                    .build();

                if (fold_region != nullptr) {
                    row_ui.rect(row_id + ".fold.box")
                        .position(50.0f, 4.0f)
                        .size(18.0f, std::max(14.0f, height - 8.0f))
                        .color(dark ? eui::Color{0.11f, 0.14f, 0.18f, 1.0f}
                                    : eui::Color{0.96f, 0.97f, 0.99f, 1.0f})
                        .radius(3.0f)
                        .border(1.0f, dark ? eui::Color{0.42f, 0.48f, 0.57f, 1.0f}
                                           : eui::Color{0.52f, 0.56f, 0.64f, 1.0f})
                        .build();
                    row_ui.text(row_id + ".fold")
                        .position(50.0f, 0.0f)
                        .size(18.0f, height)
                        .text(collapsed ? "+" : "-")
                        .fontFamily(app::fonts::ui())
                        .fontSize(16.0f)
                        .fontWeight(800)
                        .dirtyKey(collapsed ? "fold.plus" : "fold.minus")
                        .horizontalAlign(eui::HorizontalAlign::Center)
                        .verticalAlign(eui::VerticalAlign::Center)
                        .color(dark ? eui::Color{0.64f, 0.70f, 0.79f, 1.0f}
                                    : eui::Color{0.38f, 0.43f, 0.51f, 1.0f})
                        .build();
                }

                for (std::size_t span_index = 0; span_index < line.spans.size(); ++span_index) {
                    const auto& span = line.spans[span_index];
                    if (span.start >= rendered_length) break;
                    const std::size_t span_length = std::min(
                        span.length, rendered_length - span.start);
                    const float span_start_x = caretX(metrics, span.start);
                    const float span_end_x = caretX(metrics, span.start + span_length);
                    row_ui.text(row_id + ".token." + std::to_string(span_index))
                        .position(text_x_ + span_start_x, 0.0f)
                        .size(std::max(
                                  1.0f,
                                  span_end_x - span_start_x),
                              height)
                        .text(line.text.substr(span.start, span_length))
                        .fontFamily(app::fonts::code())
                        .fontSize(14.0f)
                        .verticalAlign(eui::VerticalAlign::Center)
                        .color(tokenColor(span.kind, dark))
                        .build();
                }
                if (collapsed && fold_region != nullptr) {
                    const float line_end_x = caretX(metrics, rendered_length);
                    row_ui.text(row_id + ".fold.summary")
                        .position(text_x_ + line_end_x + 9.0f, 0.0f)
                        .size(std::max(80.0f, width - text_x_ - line_end_x - 12.0f), height)
                        .text("⋯ " + std::to_string(
                            fold_region->end_line - fold_region->start_line))
                        .fontFamily(app::fonts::code())
                        .fontSize(13.0f)
                        .verticalAlign(eui::VerticalAlign::Center)
                        .color(dark ? eui::Color{0.55f, 0.61f, 0.70f, 1.0f}
                                    : eui::Color{0.43f, 0.47f, 0.55f, 1.0f})
                        .build();
                }
            })
            .build();

        const float hit_width = std::max(0.0f, viewport_width - 15.0f - text_x_);
        ui_.rect(id_ + ".hit")
            .position(x_ + text_x_, y_)
            .size(hit_width, viewport_height)
            .color({0.0f, 0.0f, 0.0f, 0.0f})
            .focusable()
            .cursor(core::CursorShape::Arrow)
            .onPress([document, selection, folds, visible_lines, scroll, row_height,
                      hit_width, &interaction](
                         const core::PointerEvent& event,
                         const core::Rect& bounds) {
                interaction.origin_y = bounds.y;
                interaction.scale = hit_width > 0.0f
                    ? std::max(0.001f, bounds.width / hit_width)
                    : 1.0f;
                interaction.origin_x = bounds.x - text_x_ * interaction.scale;
                const float local_y = std::max(
                    0.0f,
                    static_cast<float>(event.y - bounds.y) / interaction.scale);
                const auto visible_line = selection_line_at_viewport_position(
                    visible_lines->size(), row_height, *scroll, local_y);
                const auto line = (*visible_lines)[visible_line];
                const float local_x = std::max(
                    0.0f,
                    static_cast<float>(event.x - bounds.x) / interaction.scale);
                const auto byte_column = byteOffsetAtX(
                    document->lines()[line].text, local_x);

                const auto now = std::chrono::steady_clock::now();
                const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - interaction.last_press_time).count();
                const float delta_x = local_x - interaction.last_press_x;
                const float delta_y = local_y - interaction.last_press_y;
                const bool repeated = elapsed >= 0 && elapsed <= 420 &&
                    delta_x * delta_x + delta_y * delta_y <= 36.0f;
                interaction.click_count = repeated
                    ? interaction.click_count % 3 + 1
                    : 1;
                interaction.last_press_time = now;
                interaction.last_press_x = local_x;
                interaction.last_press_y = local_y;

                if (interaction.click_count == 2) {
                    selection->select_word(*document, line, byte_column);
                } else if (interaction.click_count == 3) {
                    selection->select_line(*document, line);
                } else {
                    selection->begin(*document, line, byte_column);
                }
                core::platform::requestUiUpdate();
            })
            .onDrag([document, selection, visible_lines, scroll, row_height,
                     viewport_height, &interaction](
                        const core::dsl::DragEvent& event) {
                const float top = interaction.origin_y;
                const float bottom = top + viewport_height * interaction.scale;
                const float max_scroll = std::max(
                    0.0f,
                    static_cast<float>(visible_lines->size()) * row_height - viewport_height);
                if (static_cast<float>(event.y) < top) {
                    const float distance =
                        (top - static_cast<float>(event.y)) / interaction.scale;
                    const float rows = std::clamp(
                        1.0f + distance / row_height, 1.0f, 12.0f);
                    *scroll = std::max(0.0f, *scroll - row_height * rows);
                } else if (static_cast<float>(event.y) > bottom) {
                    const float distance =
                        (static_cast<float>(event.y) - bottom) / interaction.scale;
                    const float rows = std::clamp(
                        1.0f + distance / row_height, 1.0f, 12.0f);
                    *scroll = std::min(max_scroll, *scroll + row_height * rows);
                }
                const float local_y = std::clamp(
                    (static_cast<float>(event.y) - top) / interaction.scale,
                    0.0f,
                    std::max(0.0f, viewport_height - 0.01f));
                const auto visible_line = selection_line_at_viewport_position(
                    visible_lines->size(), row_height, *scroll, local_y);
                const auto line = (*visible_lines)[visible_line];
                const float local_x = std::max(
                    0.0f,
                    (static_cast<float>(event.x) - interaction.origin_x) /
                            interaction.scale -
                        text_x_);
                selection->extend(
                    *document,
                    line,
                    byteOffsetAtX(document->lines()[line].text, local_x));
                core::platform::requestUiUpdate();
            })
            .onTextInput([document, selection, visible_lines, scroll, row_height,
                          viewport_height, on_copy](const core::KeyboardEvent& event) {
                if (event.selectAll) {
                    selection->select_all(*document);
                    core::platform::requestUiUpdate();
                }
                if (event.copy && on_copy) on_copy();
                if (event.escape) {
                    selection->clear();
                    core::platform::requestUiUpdate();
                }
                const float maximum = std::max(
                    0.0f,
                    static_cast<float>(visible_lines->size()) * row_height - viewport_height);
                float next = *scroll;
                if (event.up) next -= row_height;
                if (event.down) next += row_height;
                if (event.home) next = 0.0f;
                if (event.end) next = maximum;
                if (event.space) {
                    const float page = std::max(row_height, viewport_height - row_height);
                    next += event.shift ? -page : page;
                }
                next = std::clamp(next, 0.0f, maximum);
                if (next != *scroll) {
                    *scroll = next;
                    core::platform::requestUiUpdate();
                }
            })
            .build();

        if (ui_.isFocused(id_ + ".hit")) {
            ui_.rect(id_ + ".focus")
                .position(x_, y_)
                .size(viewport_width, viewport_height)
                .color({0.0f, 0.0f, 0.0f, 0.0f})
                .border(2.0f, tokens.primary)
                .radius(6.0f)
                .zIndex(900)
                .build();
        }

        if (folds != nullptr && !document->fold_regions().empty()) {
            const std::size_t first = std::min(
                visible_lines->size(),
                static_cast<std::size_t>(std::max(0.0f, *scroll) / row_height));
            const std::size_t count = static_cast<std::size_t>(
                std::ceil(viewport_height / row_height)) + 2;
            const std::size_t end = std::min(visible_lines->size(), first + count);
            for (std::size_t visible_index = first; visible_index < end; ++visible_index) {
                const std::size_t line = (*visible_lines)[visible_index];
                if (document->fold_region_at(line) == nullptr) continue;
                const float row_y = y_ + static_cast<float>(visible_index) * row_height - *scroll;
                ui_.rect(id_ + ".fold.hit." + std::to_string(line))
                    .position(x_ + 48.0f, row_y)
                    .size(20.0f, row_height)
                    .zIndex(100)
                    .states(
                        {0.0f, 0.0f, 0.0f, 0.0f},
                        {0.36f, 0.66f, 1.0f, 0.14f},
                        {0.36f, 0.66f, 1.0f, 0.24f})
                    .cursor(core::CursorShape::Hand)
                    .onClick([document, selection, folds, line] {
                        if (folds->toggle(*document, line)) {
                            selection->clear();
                            core::platform::requestUiUpdate();
                        }
                    })
                    .build();
            }
        }
    }

private:
    static constexpr float text_x_ = 68.0f;
    static constexpr std::size_t max_rendered_line_bytes_ = 16U * 1024U;

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

    static std::size_t byteOffsetAtX(const std::string& value, float x) {
        const std::string rendered = value.substr(
            0, std::min(value.size(), max_rendered_line_bytes_));
        const auto metrics = core::TextPrimitive::measureTextMetrics(
            rendered, app::fonts::code(), 14.0f, 400);
        const std::size_t count = std::min(
            metrics.byteIndices.size(), metrics.caretX.size());
        if (count == 0) return 0;
        std::size_t best = 0;
        float best_distance = std::fabs(x - metrics.caretX.front());
        for (std::size_t index = 1; index < count; ++index) {
            const float distance = std::fabs(x - metrics.caretX[index]);
            if (distance < best_distance) {
                best = index;
                best_distance = distance;
            }
        }
        return static_cast<std::size_t>(std::max(0, metrics.byteIndices[best]));
    }

    static eui::Color tokenColor(TokenKind kind, bool dark) {
        if (!dark) {
            switch (kind) {
            case TokenKind::Key: return {0.74f, 0.16f, 0.22f, 1.0f};
            case TokenKind::String: return {0.12f, 0.48f, 0.20f, 1.0f};
            case TokenKind::Number: return {0.05f, 0.40f, 0.66f, 1.0f};
            case TokenKind::Boolean: return {0.43f, 0.22f, 0.70f, 1.0f};
            case TokenKind::Null: return {0.36f, 0.39f, 0.45f, 1.0f};
            case TokenKind::Punctuation:
            case TokenKind::Plain: return {0.16f, 0.18f, 0.22f, 1.0f};
            case TokenKind::Tag: return {0.10f, 0.39f, 0.72f, 1.0f};
            case TokenKind::Attribute: return {0.65f, 0.30f, 0.10f, 1.0f};
            case TokenKind::Comment: return {0.42f, 0.46f, 0.51f, 1.0f};
            }
        }
        switch (kind) {
        case TokenKind::Key: return {0.96f, 0.46f, 0.50f, 1.0f};
        case TokenKind::String: return {0.55f, 0.82f, 0.55f, 1.0f};
        case TokenKind::Number: return {0.45f, 0.78f, 0.94f, 1.0f};
        case TokenKind::Boolean: return {0.71f, 0.56f, 0.98f, 1.0f};
        case TokenKind::Null: return {0.65f, 0.68f, 0.74f, 1.0f};
        case TokenKind::Punctuation:
        case TokenKind::Plain: return {0.87f, 0.89f, 0.93f, 1.0f};
        case TokenKind::Tag: return {0.45f, 0.72f, 1.0f, 1.0f};
        case TokenKind::Attribute: return {0.98f, 0.68f, 0.36f, 1.0f};
        case TokenKind::Comment: return {0.57f, 0.62f, 0.69f, 1.0f};
        }
        return {0.87f, 0.89f, 0.93f, 1.0f};
    }

    static std::vector<std::size_t> allLines(const HighlightedDocument& document) {
        std::vector<std::size_t> lines(document.lines().size());
        for (std::size_t index = 0; index < lines.size(); ++index) lines[index] = index;
        return lines;
    }

    eui::Ui& ui_;
    std::string id_;
    std::shared_ptr<const HighlightedDocument> document_;
    TextSelection* selection_ = nullptr;
    TextFoldState* folds_ = nullptr;
    float* scroll_ = nullptr;
    ::components::theme::ThemeColorTokens tokens_ = ::components::theme::dark();
    const std::vector<SearchMatch>* search_matches_ = nullptr;
    std::size_t active_match_ = 0;
    std::function<void()> on_copy_;
    float x_ = 0.0f;
    float y_ = 0.0f;
    float width_ = 320.0f;
    float height_ = 220.0f;
    float row_height_ = 25.0f;
    bool dark_ = true;
};

inline RichTextViewBuilder richTextView(eui::Ui& ui, const std::string& id) {
    return RichTextViewBuilder(ui, id);
}

} // namespace zeus::app_components
