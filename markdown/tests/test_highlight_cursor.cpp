#include "test_helper.hpp"
#include "markdown/highlight.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    // Test 1: Cursor at beginning of single line — inverted
    {
        auto element = highlight_markdown_with_cursor("hello", 0, true, false);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(0, 0).inverted);
        ASSERT_TRUE(!screen.PixelAt(1, 0).inverted);
    }

    // Test 2: Cursor at end of line — renders inverted space
    {
        auto element = highlight_markdown_with_cursor("hi", 2, true, false);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(2, 0).inverted);
    }

    // Test 3: Cursor in middle
    {
        auto element = highlight_markdown_with_cursor("abcde", 2, true, false);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(!screen.PixelAt(0, 0).inverted);
        ASSERT_TRUE(!screen.PixelAt(1, 0).inverted);
        ASSERT_TRUE(screen.PixelAt(2, 0).inverted);
        ASSERT_TRUE(!screen.PixelAt(3, 0).inverted);
    }

    // Test 4: Not focused/hovered — no inverted cursor
    {
        auto element = highlight_markdown_with_cursor("hello", 0, false, false);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(!screen.PixelAt(0, 0).inverted);
    }

    // Test 5: Multi-line — cursor on second line
    {
        // "abc\ndef", cursor at byte 4 = start of "def"
        auto element = highlight_markdown_with_cursor("abc\ndef", 4, true, false);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(2));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(0, 1).inverted);   // 'd' on row 1
        ASSERT_TRUE(!screen.PixelAt(0, 0).inverted);  // 'a' on row 0
    }

    // Test 6: Line numbers displayed (unfocused to avoid focus+vbox interaction)
    {
        auto element = highlight_markdown_with_cursor(
            "line1\nline2", 0, false, false, true);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(2));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "1");
        ASSERT_CONTAINS(output, "2");
        ASSERT_CONTAINS(output, "line1");
        ASSERT_CONTAINS(output, "line2");
    }

    // Test 7: Syntax markers highlighted on cursor line
    {
        auto element = highlight_markdown_with_cursor(
            "# heading", 2, true, false);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // "# " at positions 0,1 should be dim (syntax highlighted)
        ASSERT_TRUE(screen.PixelAt(0, 0).dim);
        ASSERT_TRUE(screen.PixelAt(1, 0).dim);
        // Cursor at 2 should be inverted
        ASSERT_TRUE(screen.PixelAt(2, 0).inverted);
    }

    // Test 8: Theme parameter affects syntax style
    {
        // Colorful theme uses Green for syntax (not dim)
        auto element = highlight_markdown_with_cursor(
            "# heading", 2, true, false, false, theme_colorful());
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // Colorful syntax is NOT dim
        ASSERT_TRUE(!screen.PixelAt(0, 0).dim);
    }

    // Test 9: Cursor on multi-byte UTF-8 character
    {
        // "café" — cursor at byte 3 (start of é, a 2-byte sequence)
        auto element = highlight_markdown_with_cursor(
            "caf\xC3\xA9", 3, true, false);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(3, 0).inverted);
        ASSERT_TRUE(!screen.PixelAt(2, 0).inverted);
    }

    // Test 10: Empty text with cursor
    {
        auto element = highlight_markdown_with_cursor("", 0, true, false);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(0, 0).inverted);
    }

    // Test 11: Hovered (but not focused) shows cursor
    {
        auto element = highlight_markdown_with_cursor("abc", 1, false, true);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(1, 0).inverted);
    }

    return 0;
}
