#include "markdown/editor.hpp"
#include "markdown/highlight.hpp"
#include "markdown/text_utils.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>

namespace markdown {

Editor::Editor() = default;

std::string const& Editor::content() const {
    return _content;
}

void Editor::set_content(std::string text) {
    _content = std::move(text);
}

void Editor::update_cursor_info() {
    _total_lines = 1;
    for (char c : _content) {
        if (c == '\n') ++_total_lines;
    }

    int remaining = std::min(_cursor_pos,
                             static_cast<int>(_content.size()));
    size_t start = 0;
    int line_num = 1;
    _cursor_line = 1;
    _cursor_col = 1;
    while (start < _content.size()) {
        auto nl = _content.find('\n', start);
        int line_len = (nl == std::string::npos)
            ? static_cast<int>(_content.size() - start)
            : static_cast<int>(nl - start);
        if (remaining <= line_len) {
            _cursor_line = line_num;
            // Count UTF-8 characters (not bytes) for the column
            _cursor_col = utf8_char_count(std::string_view(
                _content.data() + start, remaining)) + 1;
            break;
        }
        remaining -= line_len + 1;
        start = (nl == std::string::npos) ? _content.size() : nl + 1;
        ++line_num;
        if (start >= _content.size()) {
            _cursor_line = line_num;
            _cursor_col = 1;
        }
    }
}

// Selectable wrapper: gates keyboard events based on _selected.
// When not selected, returns false so the parent container navigates.
namespace {
class SelectableWrap : public ftxui::ComponentBase {
    bool& _selected;
public:
    SelectableWrap(ftxui::Component child, bool& selected)
        : _selected(selected) {
        Add(std::move(child));
    }

    bool Focusable() const override { return true; }

    bool OnEvent(ftxui::Event event) override {
        if (event.is_mouse()) {
            if (event.mouse().button == ftxui::Mouse::Left &&
                event.mouse().motion == ftxui::Mouse::Pressed) {
                _selected = true;
                TakeFocus();
            }
            return ComponentBase::OnEvent(event);
        }
        if (_selected) {
            if (event == ftxui::Event::Escape) {
                _selected = false;
                return true;
            }
            if (event == ftxui::Event::Tab || event == ftxui::Event::TabReverse) {
                ComponentBase::OnEvent(event);
                return true; // consume Tab while selected
            }
            return ComponentBase::OnEvent(event);
        }
        if (event == ftxui::Event::Return) {
            _selected = true;
            return true;
        }
        return false;
    }
};
} // namespace

ftxui::Component Editor::component() {
    if (_component) return _component;

    auto input_option = ftxui::InputOption();
    input_option.multiline = true;
    input_option.cursor_position = &_cursor_pos;
    input_option.transform = [this](ftxui::InputState state) {
        if (state.is_placeholder) return state.element;
        update_cursor_info();
        auto element = highlight_markdown_with_cursor(
            _content, _cursor_pos, state.focused, state.hovered, true);
        return element | ftxui::reflect(_editor_box);
    };
    auto input = ftxui::Input(&_content, input_option);

    // Intercept left-press mouse clicks to fix cursor positioning
    // (Input's internal cursor_box_ is lost when transform replaces the element).
    auto inner = ftxui::CatchEvent(input, [this, input](ftxui::Event event) {
        if (!event.is_mouse()) return false;

        auto& mouse = event.mouse();

        if (mouse.button != ftxui::Mouse::Left ||
            mouse.motion != ftxui::Mouse::Pressed ||
            !_editor_box.Contain(mouse.x, mouse.y)) {
            return false;
        }

        input->TakeFocus();

        int click_y = mouse.y - _editor_box.y_min;
        int click_x = mouse.x - _editor_box.x_min;

        // Split document into lines
        std::vector<std::string> lines;
        std::istringstream iss(_content);
        std::string line;
        while (std::getline(iss, line)) lines.push_back(line);
        if (lines.empty()) lines.push_back("");

        // Subtract gutter columns from click_x
        click_x -= gutter_chars(static_cast<int>(lines.size()));
        click_x = std::max(0, click_x);

        click_y = std::clamp(click_y, 0, static_cast<int>(lines.size()) - 1);
        // click_x is a visual column — clamp to character count, not byte count
        int max_col = utf8_char_count(lines[click_y]);
        click_x = std::clamp(click_x, 0, max_col);

        // Convert visual column to byte offset within the line
        size_t byte_x = utf8_char_to_byte(lines[click_y], click_x);

        int pos = 0;
        for (int i = 0; i < click_y; ++i) {
            pos += static_cast<int>(lines[i].size()) + 1;
        }
        pos += static_cast<int>(byte_x);
        _cursor_pos = std::min(pos, static_cast<int>(_content.size()));

        return true;
    });

    // Wrap with selectable behavior
    _component = std::make_shared<SelectableWrap>(inner, _selected);
    return _component;
}

} // namespace markdown
