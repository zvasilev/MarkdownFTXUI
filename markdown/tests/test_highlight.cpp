#include "test_helper.hpp"
#include "markdown/highlight.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    // Test 1: Bold markers are highlighted, text is normal
    {
        auto element = highlight_markdown_syntax("**bold** and *italic*");
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "bold");
        ASSERT_CONTAINS(output, "italic");

        // ** at position 0,1 should be dim (syntax highlighted)
        ASSERT_TRUE(screen.PixelAt(0, 0).dim);
        ASSERT_TRUE(screen.PixelAt(1, 0).dim);
        // "bold" at position 2 should NOT be dim
        ASSERT_TRUE(!screen.PixelAt(2, 0).dim);
        // ** at position 6,7 should be dim
        ASSERT_TRUE(screen.PixelAt(6, 0).dim);
        ASSERT_TRUE(screen.PixelAt(7, 0).dim);
    }

    // Test 2: Link markers are highlighted
    {
        auto element = highlight_markdown_syntax("[link](url)");
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        // [ at 0, ] at 5, ( at 6, ) at 10 should be dim
        ASSERT_TRUE(screen.PixelAt(0, 0).dim);   // [
        ASSERT_TRUE(!screen.PixelAt(1, 0).dim);   // l
        ASSERT_TRUE(screen.PixelAt(5, 0).dim);   // ]
        ASSERT_TRUE(screen.PixelAt(6, 0).dim);   // (
        ASSERT_TRUE(screen.PixelAt(10, 0).dim);  // )
    }

    // Test 3: Heading marker highlighted
    {
        auto element = highlight_markdown_syntax("# Heading");
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        // "# " (positions 0,1) should be dim
        ASSERT_TRUE(screen.PixelAt(0, 0).dim);
        ASSERT_TRUE(screen.PixelAt(1, 0).dim);
        // "H" at position 2 should NOT be dim
        ASSERT_TRUE(!screen.PixelAt(2, 0).dim);
    }

    // Test 4: Blockquote marker highlighted
    {
        auto element = highlight_markdown_syntax("> quote");
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        // "> " should be dim
        ASSERT_TRUE(screen.PixelAt(0, 0).dim);
        ASSERT_TRUE(screen.PixelAt(1, 0).dim);
        // "q" at 2 should NOT be dim
        ASSERT_TRUE(!screen.PixelAt(2, 0).dim);
    }

    // Test 5: List marker highlighted
    {
        auto element = highlight_markdown_syntax("- item");
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        // "- " should be dim
        ASSERT_TRUE(screen.PixelAt(0, 0).dim);
        ASSERT_TRUE(screen.PixelAt(1, 0).dim);
        // "i" at 2 should NOT be dim
        ASSERT_TRUE(!screen.PixelAt(2, 0).dim);
    }

    // Test 6: Backtick markers highlighted
    {
        auto element = highlight_markdown_syntax("use `code` here");
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        // ` at position 4 and 9 should be dim
        ASSERT_TRUE(screen.PixelAt(4, 0).dim);
        ASSERT_TRUE(screen.PixelAt(9, 0).dim);
        // "code" at 5 should NOT be dim
        ASSERT_TRUE(!screen.PixelAt(5, 0).dim);
    }

    // Test 7: Multi-line text
    {
        auto element = highlight_markdown_syntax("# Title\n\nPlain text");
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Title");
        ASSERT_CONTAINS(output, "Plain text");
    }

    // Test 8: Ordered list marker highlighted
    {
        auto element = highlight_markdown_syntax("1. item");
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // "1. " (3 chars) should be dim
        ASSERT_TRUE(screen.PixelAt(0, 0).dim);  // 1
        ASSERT_TRUE(screen.PixelAt(1, 0).dim);  // .
        ASSERT_TRUE(screen.PixelAt(2, 0).dim);  // space
        // "i" at 3 should NOT be dim
        ASSERT_TRUE(!screen.PixelAt(3, 0).dim);
    }

    // Test 9: Code fence highlighted as syntax
    {
        auto element = highlight_markdown_syntax("```python");
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // Entire line should be dim (all syntax)
        ASSERT_TRUE(screen.PixelAt(0, 0).dim);  // `
        ASSERT_TRUE(screen.PixelAt(3, 0).dim);  // p
        ASSERT_TRUE(screen.PixelAt(8, 0).dim);  // n
    }

    // Test 10: Thematic break highlighted
    {
        auto element = highlight_markdown_syntax("---");
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(0, 0).dim);
        ASSERT_TRUE(screen.PixelAt(1, 0).dim);
        ASSERT_TRUE(screen.PixelAt(2, 0).dim);
    }

    // Test 11: Image marker ! highlighted
    {
        auto element = highlight_markdown_syntax("![alt](url)");
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // ! at 0, [ at 1, ] at 5, ( at 6, ) at 10 should be dim
        ASSERT_TRUE(screen.PixelAt(0, 0).dim);   // !
        ASSERT_TRUE(screen.PixelAt(1, 0).dim);   // [
        ASSERT_TRUE(!screen.PixelAt(2, 0).dim);  // a (content)
        ASSERT_TRUE(screen.PixelAt(5, 0).dim);   // ]
        ASSERT_TRUE(screen.PixelAt(6, 0).dim);   // (
    }

    return 0;
}
