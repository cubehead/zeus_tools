#pragma once

#include "components/scroll.h"
#include "core/dsl.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace zeus::app_components {

// EUI's stock VirtualList intentionally keys row composition only by scroll
// position. Structured-text folding also changes the visible row mapping, so
// this small project-local variant exposes a content key for that revision.
class DynamicVirtualListBuilder {
public:
    using RowCompose = std::function<void(
        core::dsl::Ui&, const std::string&, std::int64_t, float, float)>;

    DynamicVirtualListBuilder(core::dsl::Ui& ui, std::string id)
        : ui_(ui), id_(std::move(id)) {}

    DynamicVirtualListBuilder& position(float x, float y) {
        x_ = x;
        y_ = y;
        return *this;
    }
    DynamicVirtualListBuilder& size(float width, float height) {
        width_ = width;
        height_ = height;
        return *this;
    }
    DynamicVirtualListBuilder& itemCount(std::int64_t value) {
        item_count_ = std::max<std::int64_t>(0, value);
        return *this;
    }
    DynamicVirtualListBuilder& rowHeight(float value) {
        row_height_ = std::max(1.0f, value);
        return *this;
    }
    DynamicVirtualListBuilder& offset(float value) {
        offset_ = std::max(0.0f, value);
        return *this;
    }
    DynamicVirtualListBuilder& step(float value) {
        step_ = std::max(1.0f, value);
        return *this;
    }
    DynamicVirtualListBuilder& overscanViewports(float value) {
        overscan_viewports_ = std::max(0.0f, value);
        return *this;
    }
    DynamicVirtualListBuilder& scrollbarWidth(float value) {
        scrollbar_width_ = std::max(0.0f, value);
        return *this;
    }
    DynamicVirtualListBuilder& scrollbarGap(float value) {
        scrollbar_gap_ = std::max(0.0f, value);
        return *this;
    }
    DynamicVirtualListBuilder& theme(
        const ::components::theme::ThemeColorTokens& tokens) {
        scroll_style_ = ::components::ScrollStyle(tokens);
        metrics_ = tokens.metrics;
        tokens_ = tokens;
        return *this;
    }
    DynamicVirtualListBuilder& transition(const core::Transition& value) {
        transition_ = value;
        return *this;
    }
    DynamicVirtualListBuilder& contentKey(std::string value) {
        content_key_ = std::move(value);
        return *this;
    }
    DynamicVirtualListBuilder& onChange(std::function<void(float)> callback) {
        on_change_ = std::move(callback);
        return *this;
    }
    DynamicVirtualListBuilder& row(RowCompose compose) {
        row_ = std::move(compose);
        return *this;
    }

    void build() {
        const float viewport_width = std::max(0.0f, width_);
        const float viewport_height = std::max(0.0f, height_);
        const double total_height_value =
            static_cast<double>(item_count_) * static_cast<double>(row_height_);
        const float total_height = static_cast<float>(total_height_value);
        const float max_offset = std::max(0.0f, total_height - viewport_height);
        const float current_offset = std::clamp(offset_, 0.0f, max_offset);
        const bool scrollable = max_offset > 0.0f;
        const float scroll_width = scrollable ? scrollbar_width_ : 0.0f;
        const float scroll_gap = scrollable ? scrollbar_gap_ : 0.0f;
        const float content_width = std::max(
            0.0f, viewport_width - scroll_width - scroll_gap);
        const float overscan = viewport_height * overscan_viewports_;
        const double first_pixel = std::max(
            0.0, static_cast<double>(current_offset) - overscan);
        const double last_pixel = std::min(
            total_height_value,
            static_cast<double>(current_offset + viewport_height + overscan));
        const std::int64_t first_index = item_count_ > 0
            ? std::clamp<std::int64_t>(
                static_cast<std::int64_t>(std::floor(first_pixel / row_height_)),
                0, item_count_ - 1)
            : 0;
        const std::int64_t last_index = item_count_ > 0
            ? std::clamp<std::int64_t>(
                static_cast<std::int64_t>(std::ceil(last_pixel / row_height_)) + 1,
                first_index, item_count_)
            : 0;
        const auto on_change = on_change_;

        ui_.stack(id_)
            .position(x_, y_)
            .size(viewport_width, viewport_height)
            .clip()
            .scrollState(id_, current_offset, max_offset, step_)
            .composeOnScrollOffsetChange()
            .onScrollOffsetChanged([on_change](float value) {
                if (on_change) on_change(value);
                core::platform::requestUiUpdate();
            })
            .content([&] {
                ui_.stack(id_ + ".window")
                    .size(content_width, viewport_height)
                    .dirtyKey(id_ + ".virtual." + content_key_)
                    .content([&] {
                        if (!row_) return;
                        const double offset = current_offset;
                        std::int64_t slot = 0;
                        for (std::int64_t index = first_index; index < last_index; ++index) {
                            const std::string row_id = id_ + ".slot." + std::to_string(slot++);
                            const float row_y = static_cast<float>(
                                static_cast<double>(index) * row_height_ - offset);
                            ui_.stack(row_id)
                                .y(row_y)
                                .size(content_width, row_height_)
                                .content([&] {
                                    row_(ui_, row_id, index, content_width, row_height_);
                                })
                                .build();
                        }
                    })
                    .build();

                if (scrollable) {
                    ::components::scroll(ui_, id_ + ".scroll")
                        .theme(tokens_)
                        .style(scroll_style_)
                        .scrollStateId(id_)
                        .x(std::max(0.0f, viewport_width - scroll_width))
                        .size(scroll_width, viewport_height)
                        .viewport(viewport_height)
                        .content(total_height)
                        .offset(current_offset)
                        .step(step_)
                        .transition(transition_)
                        .build();
                }
            })
            .build();
    }

private:
    core::dsl::Ui& ui_;
    std::string id_;
    std::string content_key_;
    ::components::ScrollStyle scroll_style_;
    ::components::theme::ThemeMetricTokens metrics_;
    ::components::theme::ThemeColorTokens tokens_ = ::components::theme::dark();
    core::Transition transition_{};
    std::function<void(float)> on_change_;
    RowCompose row_;
    std::int64_t item_count_ = 0;
    float x_ = 0.0f;
    float y_ = 0.0f;
    float width_ = 320.0f;
    float height_ = 220.0f;
    float row_height_ = 25.0f;
    float offset_ = 0.0f;
    float step_ = 75.0f;
    float overscan_viewports_ = 0.75f;
    float scrollbar_width_ = 7.0f;
    float scrollbar_gap_ = 8.0f;
};

inline DynamicVirtualListBuilder dynamicVirtualList(
    core::dsl::Ui& ui,
    const std::string& id) {
    return DynamicVirtualListBuilder(ui, id);
}

} // namespace zeus::app_components
