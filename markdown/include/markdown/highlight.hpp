#pragma once

#include <string_view>

#include <ftxui/dom/elements.hpp>

namespace markdown {

ftxui::Element highlight_markdown_syntax(std::string_view text);

// Highlighted element with cursor embedded at cursor_position.
// Use this from InputOption::transform to get both highlighting and cursor.
ftxui::Element highlight_markdown_with_cursor(std::string_view text,
                                              int cursor_position,
                                              bool focused,
                                              bool hovered);

} // namespace markdown
