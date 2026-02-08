#include "markdown/highlight.hpp"
#include "markdown/text_utils.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace markdown {
namespace {

// Cache for pre-formatted line number strings ("  1 │ ", "  2 │ ", ...).
// Reused across frames when line count is unchanged.
struct GutterCache {
    std::vector<std::string> strings;
    int cached_count = 0;
    int cached_gw = 0;

    void ensure(int count, int gw) {
        if (count == cached_count && gw == cached_gw) return;
        cached_count = count;
        cached_gw = gw;
        strings.resize(count);
        for (int i = 0; i < count; ++i) {
            std::string num = std::to_string(i + 1);
            if (static_cast<int>(num.size()) < gw) {
                num.insert(0, gw - static_cast<int>(num.size()), ' ');
            }
            num += " \u2502 ";
            strings[i] = std::move(num);
        }
    }
};
static GutterCache s_gutter_cache;

bool is_inline_syntax(char c) {
    return c == '*' || c == '_' || c == '`' || c == '!' ||
           c == '[' || c == ']' || c == '(' || c == ')';
}

// Determine how many chars at line start are syntax markers
size_t line_marker_end(std::string_view line) {
    if (line.empty()) return 0;

    // Heading: # ## ### etc.
    if (line[0] == '#') {
        size_t end = 0;
        while (end < line.size() && line[end] == '#') ++end;
        if (end < line.size() && line[end] == ' ') ++end;
        return end;
    }
    // Blockquote: > text
    if (line[0] == '>') {
        size_t end = 1;
        if (end < line.size() && line[end] == ' ') ++end;
        return end;
    }
    // Bullet list: - item
    if (line.size() >= 2 && line[0] == '-' && line[1] == ' ') {
        return 2;
    }
    // Ordered list: 1. item, 12. item, etc.
    if (line[0] >= '0' && line[0] <= '9') {
        size_t end = 0;
        while (end < line.size() && line[end] >= '0' && line[end] <= '9') ++end;
        if (end < line.size() && line[end] == '.' &&
            end + 1 < line.size() && line[end + 1] == ' ') {
            return end + 2; // digits + ". "
        }
    }
    // Code fence: ``` or ~~~
    if (line.size() >= 3 &&
        ((line[0] == '`' && line[1] == '`' && line[2] == '`') ||
         (line[0] == '~' && line[1] == '~' && line[2] == '~'))) {
        return line.size(); // entire line is syntax
    }
    // Thematic break: --- or *** or ___ (3+ of same char, optionally spaces)
    if (line[0] == '-' || line[0] == '*' || line[0] == '_') {
        char ch = line[0];
        size_t count = 0;
        bool only_marker = true;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == ch) {
                ++count;
            } else if (line[i] != ' ') {
                only_marker = false;
                break;
            }
        }
        if (only_marker && count >= 3) {
            return line.size(); // entire line is syntax
        }
    }
    return 0;
}

bool is_syntax_at(std::string_view line, size_t pos, size_t marker_end) {
    return pos < marker_end || is_inline_syntax(line[pos]);
}

// Alias for the shared UTF-8 utility in the anonymous namespace.
size_t utf8_glyph_len(char c) { return utf8_byte_length(c); }

// Highlight a single line, returning an Element
ftxui::Element highlight_line(std::string_view line,
                              ftxui::Decorator syntax_style) {
    ftxui::Elements parts;

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

    while (i < line.size()) {
        if (is_inline_syntax(line[i])) {
            flush_normal();
            size_t start = i;
            while (i < line.size() && is_inline_syntax(line[i])) ++i;
            parts.push_back(
                ftxui::text(std::string(line.data() + start, i - start))
                    | syntax_style);
        } else {
            size_t glen = utf8_glyph_len(line[i]);
            glen = std::min(glen, line.size() - i);
            normal.append(line.data() + i, glen);
            i += glen;
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
                                          ftxui::Decorator cursor_style,
                                          ftxui::Decorator syntax_style) {
    ftxui::Elements parts;
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

} // namespace

ftxui::Element highlight_markdown_syntax(std::string_view text,
                                          Theme const& theme) {
    ftxui::Elements elements;
    for (auto line : split_lines(text)) {
        elements.push_back(highlight_line(line, theme.syntax));
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
                                              bool show_line_numbers,
                                              Theme const& theme) {
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
    int gw = show_line_numbers
        ? markdown::gutter_width(static_cast<int>(lines.size()))
        : 0;
    auto gutter_style = theme.gutter;

    if (show_line_numbers) {
        s_gutter_cache.ensure(static_cast<int>(lines.size()), gw);
    }

    ftxui::Elements elements;
    for (size_t i = 0; i < lines.size(); ++i) {
        ftxui::Element line_el;
        if (static_cast<int>(i) == cursor_line) {
            line_el = highlight_line_with_cursor(lines[i], cursor_char,
                                                    cursor_style, theme.syntax);
        } else {
            line_el = highlight_line(lines[i], theme.syntax);
        }

        if (show_line_numbers) {
            line_el = ftxui::hbox({
                ftxui::text(s_gutter_cache.strings[i]) | gutter_style,
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
