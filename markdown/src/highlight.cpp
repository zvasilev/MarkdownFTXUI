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

// Return the byte length of the UTF-8 character starting at the given byte.
size_t utf8_glyph_len(char leading_byte) {
    auto b = static_cast<unsigned char>(leading_byte);
    if (b < 0x80) return 1;
    if (b < 0xC0) return 1; // continuation byte (shouldn't be leading)
    if (b < 0xE0) return 2;
    if (b < 0xF0) return 3;
    return 4;
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

// Highlight a single line with cursor embedded at cursor_idx (byte offset).
// Batches consecutive non-syntax, non-cursor glyphs into single text elements
// for performance, while still handling multi-byte UTF-8 correctly.
ftxui::Element highlight_line_with_cursor(std::string_view line,
                                          int cursor_idx,
                                          ftxui::Decorator cursor_style) {
    ftxui::Elements parts;
    auto syntax_style = ftxui::color(ftxui::Color::Yellow) | ftxui::dim;
    size_t marker_end = line_marker_end(line);

    std::string normal_buf;
    auto flush_normal = [&] {
        if (!normal_buf.empty()) {
            parts.push_back(ftxui::text(std::move(normal_buf)));
            normal_buf.clear();
        }
    };

    size_t i = 0;
    while (i < line.size()) {
        size_t glen = utf8_glyph_len(line[i]);
        glen = std::min(glen, line.size() - i); // clamp to remaining

        bool is_cursor = (static_cast<int>(i) == cursor_idx);
        bool is_syntax = is_syntax_at(line, i, marker_end);

        if (is_cursor || is_syntax) {
            flush_normal();
            auto el = ftxui::text(std::string(line.substr(i, glen)));
            if (is_syntax) el = el | syntax_style;
            if (is_cursor) el = el | cursor_style;
            parts.push_back(std::move(el));
        } else {
            normal_buf.append(line.data() + i, glen);
        }
        i += glen;
    }
    flush_normal();

    // Cursor at end of line
    if (cursor_idx >= static_cast<int>(line.size())) {
        parts.push_back(ftxui::text(" ") | cursor_style);
    }

    if (parts.empty()) {
        return ftxui::text("");
    }
    // Always use hbox so the cursor element gets its natural 1-char width.
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
                                              bool hovered,
                                              bool show_line_numbers) {
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

    // Compute line number gutter width
    int gutter_width = 0;
    if (show_line_numbers) {
        int total = static_cast<int>(lines.size());
        gutter_width = 1;
        while (total >= 10) { ++gutter_width; total /= 10; }
    }
    auto gutter_style = ftxui::dim;

    ftxui::Elements elements;
    for (size_t i = 0; i < lines.size(); ++i) {
        ftxui::Element line_el;
        if (static_cast<int>(i) == cursor_line) {
            line_el = highlight_line_with_cursor(lines[i], cursor_char, cursor_style);
        } else {
            line_el = highlight_line(lines[i]);
        }

        if (show_line_numbers) {
            std::string num = std::to_string(i + 1);
            // Right-align the number
            while (static_cast<int>(num.size()) < gutter_width) {
                num = " " + num;
            }
            num += " \u2502 ";
            line_el = ftxui::hbox({
                ftxui::text(num) | gutter_style,
                std::move(line_el),
            });
        }

        elements.push_back(std::move(line_el));
    }

    if (elements.empty()) {
        return ftxui::text("") | cursor_style;
    }
    return ftxui::vbox(std::move(elements));
}

} // namespace markdown
