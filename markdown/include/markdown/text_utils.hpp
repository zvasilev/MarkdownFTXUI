#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
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

// Decode a UTF-8 codepoint starting at data, returning the codepoint value.
inline uint32_t utf8_codepoint(const char* data, size_t len) {
    auto b = static_cast<unsigned char>(data[0]);
    if (b < 0x80) return b;
    if (b < 0xE0 && len >= 2)
        return ((b & 0x1F) << 6) | (static_cast<unsigned char>(data[1]) & 0x3F);
    if (b < 0xF0 && len >= 3)
        return ((b & 0x0F) << 12) |
               ((static_cast<unsigned char>(data[1]) & 0x3F) << 6) |
               (static_cast<unsigned char>(data[2]) & 0x3F);
    if (len >= 4)
        return ((b & 0x07) << 18) |
               ((static_cast<unsigned char>(data[1]) & 0x3F) << 12) |
               ((static_cast<unsigned char>(data[2]) & 0x3F) << 6) |
               (static_cast<unsigned char>(data[3]) & 0x3F);
    return b; // fallback
}

// Terminal display width of a Unicode codepoint.
// Returns 0 for combining/ZWJ/VS16, 2 for wide/emoji, 1 otherwise.
inline int codepoint_width(uint32_t cp) {
    // Zero-width characters
    if (cp == 0x200D) return 0;  // ZWJ
    if (cp == 0xFE0F) return 0;  // VS16 (emoji presentation selector)
    if (cp == 0xFE0E) return 0;  // VS15 (text presentation selector)
    // Combining marks (general categories Mn, Mc, Me)
    if (cp >= 0x0300 && cp <= 0x036F) return 0;  // Combining Diacriticals
    if (cp >= 0x1AB0 && cp <= 0x1AFF) return 0;  // Combining Diacriticals Extended
    if (cp >= 0x1DC0 && cp <= 0x1DFF) return 0;  // Combining Diacriticals Supplement
    if (cp >= 0x20D0 && cp <= 0x20FF) return 0;  // Combining for Symbols
    if (cp >= 0xFE20 && cp <= 0xFE2F) return 0;  // Combining Half Marks
    // Skin tone modifiers
    if (cp >= 0x1F3FB && cp <= 0x1F3FF) return 0;
    // CJK Unified Ideographs
    if (cp >= 0x4E00 && cp <= 0x9FFF) return 2;
    // CJK Extension A
    if (cp >= 0x3400 && cp <= 0x4DBF) return 2;
    // CJK Compatibility Ideographs
    if (cp >= 0xF900 && cp <= 0xFAFF) return 2;
    // Fullwidth Forms
    if (cp >= 0xFF01 && cp <= 0xFF60) return 2;
    if (cp >= 0xFFE0 && cp <= 0xFFE6) return 2;
    // CJK Extension B-F and Supplement
    if (cp >= 0x20000 && cp <= 0x2FA1F) return 2;
    // Hangul Syllables
    if (cp >= 0xAC00 && cp <= 0xD7AF) return 2;
    // CJK Radicals, Kangxi, Ideographic Description
    if (cp >= 0x2E80 && cp <= 0x303E) return 2;
    // Katakana, Hiragana, Bopomofo
    if (cp >= 0x3040 && cp <= 0x33FF) return 2;
    // Common emoji ranges (wide in terminal)
    if (cp >= 0x1F600 && cp <= 0x1F64F) return 2;  // Emoticons
    if (cp >= 0x1F300 && cp <= 0x1F5FF) return 2;  // Misc Symbols & Pictographs
    if (cp >= 0x1F680 && cp <= 0x1F6FF) return 2;  // Transport & Map
    if (cp >= 0x1F900 && cp <= 0x1F9FF) return 2;  // Supplemental Symbols
    if (cp >= 0x1FA00 && cp <= 0x1FA6F) return 2;  // Chess Symbols
    if (cp >= 0x1FA70 && cp <= 0x1FAFF) return 2;  // Symbols Extended-A
    if (cp >= 0x2600 && cp <= 0x27BF) return 2;    // Misc Symbols & Dingbats
    if (cp >= 0x2300 && cp <= 0x23FF) return 2;    // Misc Technical
    if (cp >= 0x2B50 && cp <= 0x2B55) return 2;    // Stars, circles
    return 1;
}

// Total terminal display width of a UTF-8 string.
inline int utf8_display_width(std::string_view text) {
    int width = 0;
    size_t i = 0;
    while (i < text.size()) {
        size_t len = utf8_byte_length(text[i]);
        len = std::min(len, text.size() - i);
        uint32_t cp = utf8_codepoint(text.data() + i, len);
        width += codepoint_width(cp);
        i += len;
    }
    return width;
}

// Map a terminal visual column (0-based) to a byte offset within text.
// Accounts for wide characters that occupy 2 terminal columns.
inline size_t visual_col_to_byte(std::string_view text, int col) {
    size_t byte_pos = 0;
    int visual = 0;
    while (byte_pos < text.size() && visual < col) {
        size_t len = utf8_byte_length(text[byte_pos]);
        len = std::min(len, text.size() - byte_pos);
        uint32_t cp = utf8_codepoint(text.data() + byte_pos, len);
        visual += codepoint_width(cp);
        byte_pos += len;
    }
    return byte_pos;
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

// Sanitize emoji sequences for correct FTXUI width measurement:
// 1. Strip VS16 (U+FE0F) — emoji presentation selector that desyncs widths.
// 2. Collapse ZWJ (U+200D) sequences — e.g. 🏃‍♂️ becomes just 🏃.
// Keeps ZWJ for Arabic/Indic ligatures (next codepoint < 0x2000).
inline std::string normalize_emoji_width(std::string_view text) {
    // Quick scan: if no multi-byte UTF-8, no emoji possible.
    bool needs_sanitize = false;
    for (char c : text) {
        if (static_cast<unsigned char>(c) >= 0xC0) {
            needs_sanitize = true;
            break;
        }
    }
    if (!needs_sanitize) return std::string(text);

    std::string result;
    result.reserve(text.size());
    size_t i = 0;
    while (i < text.size()) {
        size_t len = utf8_byte_length(text[i]);
        len = std::min(len, text.size() - i);
        uint32_t cp = utf8_codepoint(text.data() + i, len);

        // Strip VS16 (emoji presentation selector)
        if (cp == 0xFE0F) { i += len; continue; }

        // Collapse ZWJ sequences in emoji context
        if (cp == 0x200D && i + len < text.size()) {
            size_t next_len = utf8_byte_length(text[i + len]);
            next_len = std::min(next_len, text.size() - i - len);
            uint32_t next_cp = utf8_codepoint(text.data() + i + len, next_len);
            if (next_cp >= 0x2000) {
                // Skip both ZWJ and the following emoji codepoint
                i += len + next_len;
                continue;
            }
        }

        result.append(text.data() + i, len);
        i += len;
    }
    return result;
}

} // namespace markdown
