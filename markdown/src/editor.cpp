#include "markdown/editor.hpp"
#include "markdown/highlight.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>

namespace markdown {

Editor::Editor() = default;

std::string const& Editor::content() const {
    return content_;
}

void Editor::set_content(std::string text) {
    content_ = std::move(text);
}

void Editor::update_cursor_info() {
    total_lines_ = 1;
    for (char c : content_) {
        if (c == '\n') ++total_lines_;
    }

    int remaining = std::min(cursor_pos_,
                             static_cast<int>(content_.size()));
    size_t start = 0;
    int line_num = 1;
    cursor_line_ = 1;
    cursor_col_ = 1;
    while (start < content_.size()) {
        auto nl = content_.find('\n', start);
        int line_len = (nl == std::string::npos)
            ? static_cast<int>(content_.size() - start)
            : static_cast<int>(nl - start);
        if (remaining <= line_len) {
            cursor_line_ = line_num;
            cursor_col_ = remaining + 1;
            break;
        }
        remaining -= line_len + 1;
        start = (nl == std::string::npos) ? content_.size() : nl + 1;
        ++line_num;
        if (start >= content_.size()) {
            cursor_line_ = line_num;
            cursor_col_ = 1;
        }
    }
}

ftxui::Component Editor::component() {
    if (component_) return component_;

    auto input_option = ftxui::InputOption();
    input_option.multiline = true;
    input_option.cursor_position = &cursor_pos_;
    input_option.transform = [this](ftxui::InputState state) {
        if (state.is_placeholder) return state.element;
        update_cursor_info();
        auto element = highlight_markdown_with_cursor(
            content_, cursor_pos_, state.focused, state.hovered, true);
        return element | ftxui::reflect(editor_box_);
    };
    auto input = ftxui::Input(&content_, input_option);

    // Intercept left-press mouse clicks to fix cursor positioning
    // (Input's internal cursor_box_ is lost when transform replaces the element).
    component_ = ftxui::CatchEvent(input, [this, input](ftxui::Event event) {
        if (!event.is_mouse()) return false;

        auto& mouse = event.mouse();

        if (mouse.button != ftxui::Mouse::Left ||
            mouse.motion != ftxui::Mouse::Pressed ||
            !editor_box_.Contain(mouse.x, mouse.y)) {
            return false;
        }

        input->TakeFocus();

        int click_y = mouse.y - editor_box_.y_min;
        int click_x = mouse.x - editor_box_.x_min;

        // Split document into lines
        std::vector<std::string> lines;
        std::istringstream iss(content_);
        std::string line;
        while (std::getline(iss, line)) lines.push_back(line);
        if (lines.empty()) lines.push_back("");

        // Compute gutter width to offset click_x
        int total = static_cast<int>(lines.size());
        int gutter_width = 1;
        while (total >= 10) { ++gutter_width; total /= 10; }
        int gutter_chars = gutter_width + 3; // " N │ "
        click_x -= gutter_chars;
        click_x = std::max(0, click_x);

        click_y = std::clamp(click_y, 0, static_cast<int>(lines.size()) - 1);
        click_x = std::clamp(click_x, 0, static_cast<int>(lines[click_y].size()));

        int pos = 0;
        for (int i = 0; i < click_y; ++i) {
            pos += static_cast<int>(lines[i].size()) + 1;
        }
        pos += click_x;
        cursor_pos_ = std::min(pos, static_cast<int>(content_.size()));

        return true;
    });

    return component_;
}

} // namespace markdown
