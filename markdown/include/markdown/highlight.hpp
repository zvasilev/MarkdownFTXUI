#pragma once

#include <string_view>

#include <ftxui/dom/elements.hpp>

#include "markdown/theme.hpp"

namespace markdown {

ftxui::Element highlight_markdown_syntax(
    std::string_view text,
    Theme const& theme = theme_default());

// Highlighted element with cursor embedded at cursor_position.
// Use this from InputOption::transform to get both highlighting and cursor.
// When show_line_numbers is true, a gutter with line numbers is prepended.
ftxui::Element highlight_markdown_with_cursor(
    std::string_view text,
    int cursor_position,
    bool focused,
    bool hovered,
    bool show_line_numbers = false,
    Theme const& theme = theme_default());

} // namespace markdown
