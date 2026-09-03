#include "accessibility.h"

#include "app_controller.h"
#include "app_state.h"
#include "i18n.h"

#include "core/dsl.h"
#include "core/platform/native_bridge.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace app::accessibility {
namespace {

constexpr std::size_t kMaxAccessibleTextBytes = 256U * 1024U;

struct OwnedElement {
    std::string id;
    std::string label;
    std::string value;
    std::string help;
    eui_accessibility_element native{};
};

std::string clipped_text(const std::string& value) {
    if (value.size() <= kMaxAccessibleTextBytes) return value;
    std::size_t end = kMaxAccessibleTextBytes;
    while (end > 0 && (static_cast<unsigned char>(value[end]) & 0xC0U) == 0x80U) --end;
    return value.substr(0, end) + "\n…";
}

std::string csv_text(const AppState& state) {
    if (!state.result.csv) return {};
    std::string value;
    for (const auto& row : state.result.csv->rows) {
        for (std::size_t column = 0; column < row.size(); ++column) {
            if (column != 0) value.push_back('\t');
            value += row[column];
            if (value.size() >= kMaxAccessibleTextBytes) return clipped_text(value);
        }
        value.push_back('\n');
        if (value.size() >= kMaxAccessibleTextBytes) return clipped_text(value);
    }
    return value;
}

std::string result_text(const AppState& state) {
    if (state.result.csv) return csv_text(state);
    if (state.result.document) return clipped_text(state.result.document->text());
    return {};
}

std::string trim_disclosure(std::string value) {
    if (value.rfind("▾ ", 0) == 0 || value.rfind("▸ ", 0) == 0) value.erase(0, 4);
    return value;
}

bool has_suffix(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() &&
        value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string sibling_text(const eui::Ui& ui, const std::string& target_id) {
    std::string id = target_id;
    const auto suffix = id.rfind('.');
    if (suffix == std::string::npos) return {};
    id.erase(suffix);
    const char* candidate_suffix = target_id.compare(suffix, 6, ".field") == 0
        ? ".label" : ".text";
    if (const core::dsl::Element* text = ui.find(id + candidate_suffix)) {
        return trim_disclosure(text->text);
    }
    return {};
}

std::string fallback_label(const std::string& id) {
    const std::unordered_map<std::string, std::string> labels{
        {"header.open.bg", controller::tr(i18n::Text::OpenFile)},
        {"header.export.bg", controller::tr(i18n::Text::ExportResult)},
        {"header.theme.bg", controller::tr(i18n::Text::Theme)},
        {"header.language.field", controller::tr(i18n::Text::Language)},
        {"header.about.bg", controller::tr(i18n::Text::About)},
        {"input.editor.hit", controller::tr(i18n::Text::MessageInput)},
        {"bottom.search.hit", controller::tr(i18n::Text::SearchResult)},
        {"bottom.search.case.bg", "Aa"},
        {"bottom.search.regex.bg", ".*"},
        {"bottom.previous.bg", controller::tr(i18n::Text::PreviousMatch)},
        {"bottom.next.bg", controller::tr(i18n::Text::NextMatch)},
    };
    for (const auto& item : labels) {
        if (has_suffix(id, item.first)) return item.second;
    }
    return {};
}

void collect_focusable(
    const core::dsl::Element& element,
    std::vector<const core::dsl::Element*>& out) {
    if (element.focusable && !element.disabled && element.frame.width > 0.0f &&
        element.frame.height > 0.0f) out.push_back(&element);
    for (const auto& child : element.children) collect_focusable(*child, out);
}

int role_for(const std::string& id) {
    if (id.size() >= 4 && id.compare(id.size() - 4, 4, ".hit") == 0) {
        return has_suffix(id, "input.editor.hit") ? EUI_ACCESSIBILITY_TEXT_AREA
                                                    : EUI_ACCESSIBILITY_TEXT_FIELD;
    }
    if (id.size() >= 6 && id.compare(id.size() - 6, 6, ".field") == 0) {
        return EUI_ACCESSIBILITY_POPUP_BUTTON;
    }
    return EUI_ACCESSIBILITY_BUTTON;
}

void add_owned(std::vector<OwnedElement>& owned, std::string id, std::string label,
               std::string value, std::string help, int role,
               const core::LayoutRect& frame, bool pressable) {
    owned.push_back({});
    OwnedElement& item = owned.back();
    item.id = std::move(id);
    item.label = std::move(label);
    item.value = std::move(value);
    item.help = std::move(help);
    item.native.role = role;
    item.native.x = frame.x;
    item.native.y = frame.y;
    item.native.width = frame.width;
    item.native.height = frame.height;
    item.native.enabled = 1;
    item.native.pressable = pressable ? 1 : 0;
}

} // namespace

void publish(eui::Ui& ui, const eui::Screen& screen) {
    // Runtime will repeat this cheap pass after compose. Doing it here keeps
    // accessibility frames derived from the actual DSL tree, not duplicated UI constants.
    ui.layout(screen);
    const AppState& state = controller::state();
    std::vector<const core::dsl::Element*> focusable;
    for (const auto& root : ui.roots()) collect_focusable(*root, focusable);

    std::vector<OwnedElement> owned;
    owned.reserve(focusable.size() + 1U);
    std::string about_scope;
    if (state.about_dialog_open) {
        if (const core::dsl::Element* dialog = ui.find("about.dialog")) {
            about_scope = dialog->id;
            const auto suffix = about_scope.rfind(".dialog");
            if (suffix != std::string::npos) about_scope.erase(suffix);
            about_scope.push_back('.');
        }
    }
    for (const core::dsl::Element* element : focusable) {
        const bool belongs_to_about = !about_scope.empty() &&
            element->id.rfind(about_scope, 0) == 0;
        if (state.about_dialog_open && !belongs_to_about) continue;
        std::string label = fallback_label(element->id);
        if (label.empty()) label = sibling_text(ui, element->id);
        if (label.empty()) continue;
        std::string value;
        if (has_suffix(element->id, "input.editor.hit")) value = clipped_text(state.input_text);
        else if (has_suffix(element->id, "bottom.search.hit")) value = state.search.query;
        else if (has_suffix(element->id, "header.language.field")) {
            if (const core::dsl::Element* text = ui.find("header.language.label")) value = text->text;
        }
        const int role = role_for(element->id);
        add_owned(owned, element->id, std::move(label), std::move(value), {}, role,
                  element->frame, true);
    }

    if (const core::dsl::Element* result = ui.find("result.background")) {
        if (!state.about_dialog_open) {
            add_owned(owned, result->id, controller::tr(i18n::Text::MessageResult),
                      result_text(state), state.result.status,
                      EUI_ACCESSIBILITY_TEXT_AREA, result->frame, false);
        }
    }

    if (state.about_dialog_open) {
        static const char* text_ids[] = {
            "about.title", "about.version", "about.description", "about.privacy.text",
            "about.thanks.title", "about.thanks.description", "about.license"
        };
        for (const char* id : text_ids) {
            if (const core::dsl::Element* text = ui.find(id)) {
                add_owned(owned, text->id, {}, text->text, {},
                          EUI_ACCESSIBILITY_STATIC_TEXT, text->frame, false);
            }
        }
    }

    std::stable_sort(owned.begin(), owned.end(), [](const OwnedElement& left,
                                                     const OwnedElement& right) {
        if (left.native.y != right.native.y) return left.native.y < right.native.y;
        return left.native.x < right.native.x;
    });

    std::vector<eui_accessibility_element> native;
    native.reserve(owned.size());
    for (OwnedElement& item : owned) {
        item.native.id = item.id.c_str();
        item.native.label = item.label.c_str();
        item.native.value = item.value.c_str();
        item.native.help = item.help.c_str();
        native.push_back(item.native);
    }
    eui_set_accessibility_elements(native.data(), static_cast<int>(native.size()), screen.height);
}

} // namespace app::accessibility
