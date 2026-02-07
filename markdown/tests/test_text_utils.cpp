#include "test_helper.hpp"
#include "markdown/text_utils.hpp"

using namespace markdown;

int main() {
    // --- utf8_byte_length ---

    // ASCII characters: 1 byte
    ASSERT_EQ(utf8_byte_length('A'), 1u);
    ASSERT_EQ(utf8_byte_length(' '), 1u);
    ASSERT_EQ(utf8_byte_length('~'), 1u);
    ASSERT_EQ(utf8_byte_length('\0'), 1u);

    // 2-byte leading bytes (0xC0-0xDF)
    ASSERT_EQ(utf8_byte_length('\xC3'), 2u); // e.g. é
    ASSERT_EQ(utf8_byte_length('\xD0'), 2u); // e.g. Cyrillic

    // 3-byte leading bytes (0xE0-0xEF)
    ASSERT_EQ(utf8_byte_length('\xE9'), 3u); // e.g. CJK
    ASSERT_EQ(utf8_byte_length('\xE2'), 3u); // e.g. bullet •

    // 4-byte leading bytes (0xF0-0xFF)
    ASSERT_EQ(utf8_byte_length('\xF0'), 4u); // e.g. emoji

    // Continuation bytes (0x80-0xBF) — treated as 1 byte (error recovery)
    ASSERT_EQ(utf8_byte_length('\x80'), 1u);
    ASSERT_EQ(utf8_byte_length('\xBF'), 1u);

    // --- utf8_char_count ---

    ASSERT_EQ(utf8_char_count(""), 0);
    ASSERT_EQ(utf8_char_count("hello"), 5);
    ASSERT_EQ(utf8_char_count("a b"), 3);

    // café: c(1) a(1) f(1) é(2) = 4 chars, 5 bytes
    ASSERT_EQ(utf8_char_count("caf\xC3\xA9"), 4);

    // 重要: 2 CJK chars, 6 bytes
    ASSERT_EQ(utf8_char_count("\xE9\x87\x8D\xE8\xA6\x81"), 2);

    // Mixed: a(1) + é(2) + 重(3) = 3 chars, 6 bytes
    ASSERT_EQ(utf8_char_count("a\xC3\xA9\xE9\x87\x8D"), 3);

    // --- utf8_char_to_byte ---

    // ASCII-only
    ASSERT_EQ(utf8_char_to_byte("hello", 0), 0u);
    ASSERT_EQ(utf8_char_to_byte("hello", 3), 3u);
    ASSERT_EQ(utf8_char_to_byte("hello", 5), 5u);

    // café: char 3 = start of é at byte 3, char 4 = past é at byte 5
    ASSERT_EQ(utf8_char_to_byte("caf\xC3\xA9", 0), 0u);
    ASSERT_EQ(utf8_char_to_byte("caf\xC3\xA9", 3), 3u);
    ASSERT_EQ(utf8_char_to_byte("caf\xC3\xA9", 4), 5u);

    // Beyond end clamps to string size
    ASSERT_EQ(utf8_char_to_byte("ab", 5), 2u);
    ASSERT_EQ(utf8_char_to_byte("", 3), 0u);

    // CJK: char 0 = byte 0, char 1 = byte 3
    ASSERT_EQ(utf8_char_to_byte("\xE9\x87\x8D\xE8\xA6\x81", 0), 0u);
    ASSERT_EQ(utf8_char_to_byte("\xE9\x87\x8D\xE8\xA6\x81", 1), 3u);
    ASSERT_EQ(utf8_char_to_byte("\xE9\x87\x8D\xE8\xA6\x81", 2), 6u);

    // --- gutter_width ---

    ASSERT_EQ(gutter_width(1), 1);
    ASSERT_EQ(gutter_width(9), 1);
    ASSERT_EQ(gutter_width(10), 2);
    ASSERT_EQ(gutter_width(99), 2);
    ASSERT_EQ(gutter_width(100), 3);
    ASSERT_EQ(gutter_width(999), 3);
    ASSERT_EQ(gutter_width(1000), 4);

    // --- gutter_chars = gutter_width + 3 ---

    ASSERT_EQ(gutter_chars(1), 4);   // 1 digit + " │ " = 4
    ASSERT_EQ(gutter_chars(10), 5);  // 2 digits + " │ " = 5
    ASSERT_EQ(gutter_chars(100), 6); // 3 digits + " │ " = 6

    // --- codepoint_width ---

    ASSERT_EQ(codepoint_width('A'), 1);
    ASSERT_EQ(codepoint_width(0x00E9), 1);  // é — Latin
    // CJK Unified Ideograph U+4E16 (世)
    ASSERT_EQ(codepoint_width(0x4E16), 2);
    // Fullwidth exclamation U+FF01
    ASSERT_EQ(codepoint_width(0xFF01), 2);
    // Hangul syllable U+AC00
    ASSERT_EQ(codepoint_width(0xAC00), 2);

    // --- utf8_display_width ---

    ASSERT_EQ(utf8_display_width(""), 0);
    ASSERT_EQ(utf8_display_width("hello"), 5);
    // "世界" = 2 wide chars = 4 columns
    ASSERT_EQ(utf8_display_width("\xE4\xB8\x96\xE7\x95\x8C"), 4);
    // "a世b" = 1 + 2 + 1 = 4 columns
    ASSERT_EQ(utf8_display_width("a\xE4\xB8\x96""b"), 4);
    // café: c(1) a(1) f(1) é(1) = 4 columns (all narrow)
    ASSERT_EQ(utf8_display_width("caf\xC3\xA9"), 4);

    // --- visual_col_to_byte ---

    // ASCII: col=byte
    ASSERT_EQ(visual_col_to_byte("hello", 0), 0u);
    ASSERT_EQ(visual_col_to_byte("hello", 3), 3u);
    // "a世b": col 0='a'(byte 0), col 1='世'(byte 1), col 3='b'(byte 4)
    ASSERT_EQ(visual_col_to_byte("a\xE4\xB8\x96""b", 0), 0u);
    ASSERT_EQ(visual_col_to_byte("a\xE4\xB8\x96""b", 1), 1u);
    ASSERT_EQ(visual_col_to_byte("a\xE4\xB8\x96""b", 3), 4u);
    // col 2 is inside '世' — should land at byte 4 (past the wide char)
    ASSERT_EQ(visual_col_to_byte("a\xE4\xB8\x96""b", 2), 4u);

    return 0;
}
