#include "markdown/highlight.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace markdown {
namespace {

bool is_inline_syntax(char c) {
    return c == '*' || c == '_' || c == '`' ||
           c == '[' || c == ']' || c == '(' || c == ')';
}

// Determine how many chars at line start are syntax markers
size_t line_marker_end(std::string_view line) {
    if (!line.empty() && line[0] == '#') {
        size_t end = 0;
        while (end < line.size() && line[end] == '#') ++end;
        if (end < line.size() && line[end] == ' ') ++end;
        return end;
    }
    if (!line.empty() && line[0] == '>') {
        size_t end = 1;
        if (end < line.size() && line[end] == ' ') ++end;
        return end;
    }
    if (line.size() >= 2 && line[0] == '-' && line[1] == ' ') {
        return 2;
    }
    return 0;
}

bool is_syntax_at(std::string_view line, size_t pos, size_t marker_end) {
    return pos < marker_end || is_inline_syntax(line[pos]);
}

// Highlight a single line, returning an Element
ftxui::Element highlight_line(std::string_view line) {
    ftxui::Elements parts;
    auto syntax_style = ftxui::color(ftxui::Color::Yellow) | ftxui::dim;

    size_t i = 0;
    size_t marker_end = line_marker_end(line);

    // Emit start-of-line markers as one chunk
    if (marker_end > 0) {
        parts.push_back(ftxui::text(std::string(line.substr(0, marker_end))) | syntax_style);
        i = marker_end;
    }

    // Process remaining characters for inline syntax
    std::string normal;
    auto flush_normal = [&] {
        if (!normal.empty()) {
            parts.push_back(ftxui::text(normal));
            normal.clear();
        }
    };

    for (; i < line.size(); ++i) {
        if (is_inline_syntax(line[i])) {
            flush_normal();
            parts.push_back(ftxui::text(std::string(1, line[i])) | syntax_style);
        } else {
            normal += line[i];
        }
    }
    flush_normal();

    if (parts.empty()) {
        return ftxui::text("");
    }
    if (parts.size() == 1) {
        return std::move(parts[0]);
    }
    return ftxui::hbox(std::move(parts));
}

// Highlight a single line with cursor embedded at cursor_idx.
// Only the cursor line uses this (character-by-character) approach.
ftxui::Element highlight_line_with_cursor(std::string_view line,
                                          int cursor_idx,
                                          ftxui::Decorator cursor_style) {
    ftxui::Elements parts;
    auto syntax_style = ftxui::color(ftxui::Color::Yellow) | ftxui::dim;
    size_t marker_end = line_marker_end(line);

    for (size_t i = 0; i < line.size(); ++i) {
        auto el = ftxui::text(std::string(1, line[i]));
        if (is_syntax_at(line, i, marker_end)) {
            el = el | syntax_style;
        }
        if (static_cast<int>(i) == cursor_idx) {
            el = el | cursor_style;
        }
        parts.push_back(std::move(el));
    }

    // Cursor at end of line
    if (cursor_idx >= static_cast<int>(line.size())) {
        parts.push_back(ftxui::text(" ") | cursor_style);
    }

    if (parts.empty()) {
        return ftxui::text("");
    }
    // Always use hbox so the cursor element gets its natural 1-char width.
    // Without hbox, the parent vbox gives the element full width,
    // and FTXUI places the terminal cursor at box_.x_max (right edge).
    return ftxui::hbox(std::move(parts));
}

// Split text into lines (without newline characters)
std::vector<std::string_view> split_lines(std::string_view text) {
    std::vector<std::string_view> lines;
    size_t start = 0;
    while (start <= text.size()) {
        auto end = text.find('\n', start);
        if (end == std::string_view::npos) {
            lines.push_back(text.substr(start));
            break;
        }
        lines.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    return lines;
}

} // namespace

ftxui::Element highlight_markdown_syntax(std::string_view text) {
    ftxui::Elements elements;
    for (auto line : split_lines(text)) {
        elements.push_back(highlight_line(line));
    }
    if (elements.empty()) {
        return ftxui::text("");
    }
    return ftxui::vbox(std::move(elements));
}

ftxui::Element highlight_markdown_with_cursor(std::string_view text,
                                              int cursor_position,
                                              bool focused,
                                              bool hovered) {
    // Render cursor as inverted character + focus (for frame scrolling).
    // Using focus (hidden terminal cursor) instead of focusCursorBarBlinking
    // avoids terminal cursor flash artifacts between frames.
    ftxui::Decorator cursor_style = (!focused && !hovered)
        ? ftxui::nothing
        : ftxui::Decorator([](ftxui::Element e) {
              return e | ftxui::inverted | ftxui::focus;
          });

    auto lines = split_lines(text);

    // Find which line the cursor is on (same logic as FTXUI's Input)
    cursor_position = std::max(0, std::min(cursor_position,
                                           static_cast<int>(text.size())));
    int cursor_line = 0;
    int cursor_char = cursor_position;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (cursor_char <= static_cast<int>(lines[i].size())) {
            cursor_line = static_cast<int>(i);
            break;
        }
        cursor_char -= static_cast<int>(lines[i].size()) + 1; // +1 for \n
        cursor_line = static_cast<int>(i) + 1;
    }

    ftxui::Elements elements;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (static_cast<int>(i) == cursor_line) {
            elements.push_back(
                highlight_line_with_cursor(lines[i], cursor_char, cursor_style));
        } else {
            elements.push_back(highlight_line(lines[i]));
        }
    }

    if (elements.empty()) {
        return ftxui::text("") | cursor_style;
    }
    return ftxui::vbox(std::move(elements));
}

} // namespace markdown
