#pragma once

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <vector>

namespace markdown {

// Byte length of the UTF-8 character starting at leading_byte.
inline size_t utf8_byte_length(char leading_byte) {
    auto b = static_cast<unsigned char>(leading_byte);
    if (b < 0x80) return 1;
    if (b < 0xC0) return 1; // continuation byte (shouldn't be leading)
    if (b < 0xE0) return 2;
    if (b < 0xF0) return 3;
    return 4;
}

// Count UTF-8 characters in text.
inline int utf8_char_count(std::string_view text) {
    int chars = 0;
    size_t i = 0;
    while (i < text.size()) {
        i += utf8_byte_length(text[i]);
        ++chars;
    }
    return chars;
}

// Convert a character index (0-based) to a byte offset within text.
inline size_t utf8_char_to_byte(std::string_view text, int char_index) {
    size_t byte_pos = 0;
    int chars = 0;
    while (byte_pos < text.size() && chars < char_index) {
        byte_pos += utf8_byte_length(text[byte_pos]);
        ++chars;
    }
    return byte_pos;
}

// Number of digits needed to display total_lines as a line number.
inline int gutter_width(int total_lines) {
    int width = 1;
    while (total_lines >= 10) { ++width; total_lines /= 10; }
    return width;
}

// Total gutter column width: digits + " │ " (3 extra terminal columns).
inline int gutter_chars(int total_lines) {
    return gutter_width(total_lines) + 3;
}

// Split text into lines (without newline characters).
inline std::vector<std::string_view> split_lines(std::string_view text) {
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

} // namespace markdown
