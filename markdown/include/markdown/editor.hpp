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

    int cursor_line() const { return cursor_line_; }
    int cursor_col() const { return cursor_col_; }
    int total_lines() const { return total_lines_; }

private:
    void update_cursor_info();

    std::string content_;
    int cursor_pos_ = 0;
    int cursor_line_ = 1;
    int cursor_col_ = 1;
    int total_lines_ = 1;
    ftxui::Box editor_box_;
    ftxui::Component component_;
};

} // namespace markdown
