#pragma once

#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

namespace markdown {

class Editor {
public:
    explicit Editor();

    ftxui::Component component();

    std::string const& content() const;
    void set_content(std::string text);

    int cursor_line() const { return _cursor_line; }
    int cursor_col() const { return _cursor_col; }
    int total_lines() const { return _total_lines; }
    bool selected() const { return _selected; }

private:
    void update_cursor_info();

    std::string _content;
    int _cursor_pos = 0;
    int _cursor_line = 1;
    int _cursor_col = 1;
    int _total_lines = 1;
    bool _selected = false;
    ftxui::Box _editor_box;
    ftxui::Component _component;
};

} // namespace markdown
