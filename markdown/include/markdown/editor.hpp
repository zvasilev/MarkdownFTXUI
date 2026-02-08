#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "markdown/theme.hpp"

namespace markdown {

class Editor {
public:
    explicit Editor();
    Editor(Editor&&) = delete;
    Editor& operator=(Editor&&) = delete;

    /// Returns the FTXUI component. Created on first call, cached thereafter.
    /// Configure the object (set_content, set_theme, etc.) before OR after
    /// this call — the renderer reads live state each frame.
    /// However, do not move this object after calling component().
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
    std::vector<std::string_view> const& cached_lines();

    std::string _content;
    int _cursor_pos = 0;
    int _cursor_line = 1;
    int _cursor_col = 1;
    int _total_lines = 1;
    bool _active = false;
    // Cache guards for update_cursor_info()
    const char* _ci_data = nullptr;
    size_t _ci_size = 0;
    int _ci_cursor = -1;
    Theme _theme{theme_default()};
    ftxui::Box _editor_box;
    ftxui::Component _component;
    // Cached split_lines result
    std::vector<std::string_view> _cached_lines;
    const char* _lines_data = nullptr;
    size_t _lines_size = 0;
    // Highlight cache — skip re-highlighting when nothing changed
    ftxui::Element _cached_highlight;
    const char* _hl_data = nullptr;
    size_t _hl_size = 0;
    int _hl_cursor = -1;
    bool _hl_focused = false;
    bool _hl_hovered = false;
};

} // namespace markdown
