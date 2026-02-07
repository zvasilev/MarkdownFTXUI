#include "test_helper.hpp"
#include "markdown/dom_builder.hpp"
#include "markdown/parser.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Parse content with two links
    auto ast = parser->parse(
        "before [link1](https://one.com) middle [link2](https://two.com) after");

    // Test 1: link_targets() has correct count and URLs
    {
        builder.build(ast, -1);
        auto const& targets = builder.link_targets();
        ASSERT_EQ(static_cast<int>(targets.size()), 2);
        auto it = targets.begin();
        ASSERT_EQ(it->url, "https://one.com");
        ++it;
        ASSERT_EQ(it->url, "https://two.com");
    }

    // Test 2: No focused link — links underlined but not inverted
    {
        auto element = builder.build(ast, -1);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "link1");
        ASSERT_CONTAINS(output, "link2");

        // Find link1 position: "before" (6) + flexbox gap (1) = 7
        ASSERT_TRUE(screen.PixelAt(7, 0).underlined);
        ASSERT_TRUE(!screen.PixelAt(7, 0).inverted);
    }

    // Test 3: focused_link=0 — first link inverted
    {
        auto element = builder.build(ast, 0);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        // link1 at position 7 should be inverted + underlined
        ASSERT_TRUE(screen.PixelAt(7, 0).inverted);
        ASSERT_TRUE(screen.PixelAt(7, 0).underlined);
    }

    // Test 4: focused_link=1 — second link inverted, first not
    {
        auto element = builder.build(ast, 1);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        // link1 should NOT be inverted
        ASSERT_TRUE(!screen.PixelAt(7, 0).inverted);
        ASSERT_TRUE(screen.PixelAt(7, 0).underlined);
    }

    // Test 5: link_targets() cleared on each build
    {
        auto ast2 = parser->parse("[only](https://only.com)");
        builder.build(ast2);
        ASSERT_EQ(static_cast<int>(builder.link_targets().size()), 1);
        ASSERT_EQ(builder.link_targets().begin()->url, "https://only.com");
    }

    // Test 6: Build with theme and focused link — inverted still works
    {
        auto element = builder.build(ast, 0, theme_colorful());
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(7, 0).inverted);
    }

    // Test 7: No links — link_targets() is empty
    {
        auto ast_no_links = parser->parse("Just plain text, no links.");
        builder.build(ast_no_links);
        ASSERT_EQ(static_cast<int>(builder.link_targets().size()), 0);
    }

    // Test 8: focused_link out of range — no crash, no inverted
    {
        auto element = builder.build(ast, 99);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // Neither link should be inverted
        ASSERT_TRUE(!screen.PixelAt(7, 0).inverted);
    }

    return 0;
}
