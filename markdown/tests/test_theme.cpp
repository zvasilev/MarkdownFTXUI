#include "test_helper.hpp"
#include "markdown/theme.hpp"
#include "markdown/dom_builder.hpp"
#include "markdown/parser.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Theme names are distinct
    {
        ASSERT_EQ(theme_default().name, "Default");
        ASSERT_EQ(theme_high_contrast().name, "Contrast");
        ASSERT_EQ(theme_colorful().name, "Colorful");
    }

    // Test 2: Themes return same object on repeated calls (static)
    {
        auto const* p1 = &theme_default();
        auto const* p2 = &theme_default();
        ASSERT_TRUE(p1 == p2);
        auto const* p3 = &theme_colorful();
        auto const* p4 = &theme_colorful();
        ASSERT_TRUE(p3 == p4);
    }

    // Test 3: Default theme H1 is bold + underlined
    {
        auto ast = parser->parse("# Title");
        auto element = builder.build(ast, -1, theme_default());
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(0, 0).bold);
        ASSERT_TRUE(screen.PixelAt(0, 0).underlined);
    }

    // Test 4: Default theme H3 is bold + dim
    {
        auto ast = parser->parse("### Sub");
        auto element = builder.build(ast, -1, theme_default());
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(0, 0).bold);
        ASSERT_TRUE(screen.PixelAt(0, 0).dim);
    }

    // Test 5: High contrast theme H3 is bold but NOT dim
    {
        auto ast = parser->parse("### Sub");
        auto element = builder.build(ast, -1, theme_high_contrast());
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(0, 0).bold);
        ASSERT_TRUE(!screen.PixelAt(0, 0).dim);
    }

    // Test 6: Default theme blockquote is dim
    {
        auto ast = parser->parse("> quoted text");
        auto element = builder.build(ast, -1, theme_default());
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // "│ " prefix (2 chars), text starts at position 2
        ASSERT_TRUE(screen.PixelAt(2, 0).dim);
    }

    // Test 7: High contrast theme blockquote is NOT dim
    {
        auto ast = parser->parse("> quoted text");
        auto element = builder.build(ast, -1, theme_high_contrast());
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(!screen.PixelAt(2, 0).dim);
    }

    // Test 8: Default theme inline code is inverted
    {
        auto ast = parser->parse("`code`");
        auto element = builder.build(ast, -1, theme_default());
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(0, 0).inverted);
    }

    // Test 9: Colorful theme H1 is bold + underlined (same structure)
    {
        auto ast = parser->parse("# Title");
        auto element = builder.build(ast, -1, theme_colorful());
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(0, 0).bold);
        ASSERT_TRUE(screen.PixelAt(0, 0).underlined);
    }

    // Test 10: High contrast link is bold
    {
        auto ast = parser->parse("[link](https://example.com)");
        auto element = builder.build(ast, -1, theme_high_contrast());
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(0, 0).bold);
        ASSERT_TRUE(screen.PixelAt(0, 0).underlined);
    }

    // Test 11: Default link is underlined but NOT bold
    {
        auto ast = parser->parse("[link](https://example.com)");
        auto element = builder.build(ast, -1, theme_default());
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(0, 0).underlined);
        ASSERT_TRUE(!screen.PixelAt(0, 0).bold);
    }

    return 0;
}
