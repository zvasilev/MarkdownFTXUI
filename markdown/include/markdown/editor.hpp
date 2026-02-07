#pragma once

#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "markdown/theme.hpp"

namespace markdown {

class Editor {
public:
    explicit Editor();

    ftxui::Component component();

    std::string const& content() const;
    void set_content(std::string text);

    int cursor_line() const { return _cursor_line; }
    int cursor_col() const { return _cursor_col; }
    int cursor_position() const { return _cursor_pos; }
    int total_lines() const { return _total_lines; }
    bool active() const { return _active; }

    void set_cursor_position(int byte_offset);
    void set_cursor(int line, int col);
    void set_theme(Theme const& theme) { _theme = theme; }

private:
    void update_cursor_info();

    std::string _content;
    int _cursor_pos = 0;
    int _cursor_line = 1;
    int _cursor_col = 1;
    int _total_lines = 1;
    bool _active = false;
    Theme _theme{theme_default()};
    ftxui::Box _editor_box;
    ftxui::Component _component;
};

} // namespace markdown
