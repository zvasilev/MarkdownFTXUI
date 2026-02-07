#include "markdown/editor.hpp"
#include "markdown/highlight.hpp"
#include "markdown/text_utils.hpp"

#include <algorithm>
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

void Editor::set_cursor_position(int byte_offset) {
    _cursor_pos = std::clamp(byte_offset, 0,
                             static_cast<int>(_content.size()));
}

void Editor::set_cursor(int line, int col) {
    // line and col are 1-based; col counts UTF-8 characters.
    line = std::max(1, line);
    col = std::max(1, col);

    size_t pos = 0;
    int current_line = 1;
    while (current_line < line && pos < _content.size()) {
        auto nl = _content.find('\n', pos);
        if (nl == std::string::npos) break;
        pos = nl + 1;
        ++current_line;
    }
    // pos is now at the start of the target line (or as close as we can get).
    // Find the end of this line to get its content.
    auto nl = _content.find('\n', pos);
    size_t line_end = (nl == std::string::npos) ? _content.size() : nl;
    auto line_view = std::string_view(_content.data() + pos, line_end - pos);

    // Convert 1-based character column to byte offset within the line.
    size_t byte_col = utf8_char_to_byte(line_view, col - 1);
    _cursor_pos = static_cast<int>(pos + byte_col);
}

void Editor::update_cursor_info() {
    // Fast path: skip if content and cursor haven't changed.
    if (_content.data() == _ci_data &&
        _content.size() == _ci_size &&
        _cursor_pos == _ci_cursor) {
        return;
    }
    _ci_data = _content.data();
    _ci_size = _content.size();
    _ci_cursor = _cursor_pos;

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

// Selectable wrapper: gates keyboard events based on _active.
// When not selected, returns false so the parent container navigates.
namespace {
class SelectableWrap : public ftxui::ComponentBase {
    bool& _active;
public:
    SelectableWrap(ftxui::Component child, bool& selected)
        : _active(selected) {
        Add(std::move(child));
    }

    bool Focusable() const override { return true; }

    bool OnEvent(ftxui::Event event) override {
        if (event.is_mouse()) {
            if (event.mouse().button == ftxui::Mouse::Left &&
                event.mouse().motion == ftxui::Mouse::Pressed) {
                _active = true;
                TakeFocus();
            }
            return ComponentBase::OnEvent(event);
        }
        if (_active) {
            if (event == ftxui::Event::Escape) {
                _active = false;
                return true;
            }
            if (event == ftxui::Event::Tab || event == ftxui::Event::TabReverse) {
                ComponentBase::OnEvent(event);
                return true; // consume Tab while selected
            }
            return ComponentBase::OnEvent(event);
        }
        if (event == ftxui::Event::Return) {
            _active = true;
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
            _content, _cursor_pos, state.focused, state.hovered, true,
            _theme);
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
        auto lines = split_lines(_content);
        if (lines.empty()) lines.push_back("");

        // Subtract gutter columns from click_x
        click_x -= gutter_chars(static_cast<int>(lines.size()));
        click_x = std::max(0, click_x);

        click_y = std::clamp(click_y, 0, static_cast<int>(lines.size()) - 1);
        // click_x is a visual column — clamp to display width (not char count)
        int max_col = utf8_display_width(lines[click_y]);
        click_x = std::clamp(click_x, 0, max_col);

        // Convert visual column to byte offset within the line
        size_t byte_x = visual_col_to_byte(lines[click_y], click_x);

        int pos = 0;
        for (int i = 0; i < click_y; ++i) {
            pos += static_cast<int>(lines[i].size()) + 1;
        }
        pos += static_cast<int>(byte_x);
        _cursor_pos = std::min(pos, static_cast<int>(_content.size()));

        return true;
    });

    // Wrap with selectable behavior
    _component = std::make_shared<SelectableWrap>(inner, _active);
    return _component;
}

} // namespace markdown
