#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: ***both*** parses as Emphasis → Strong → Text (cmark-gfm order)
    {
        auto ast = parser->parse("***both***");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 1u);

        auto& emph = para.children[0];
        ASSERT_EQ(emph.type, NodeType::Emphasis);
        ASSERT_EQ(emph.children.size(), 1u);

        auto& strong = emph.children[0];
        ASSERT_EQ(strong.type, NodeType::Strong);
        ASSERT_EQ(strong.children.size(), 1u);
        ASSERT_EQ(strong.children[0].text, "both");
    }

    // Test 2: ***both*** renders bold + italic
    {
        auto ast = parser->parse("***both***");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        auto& pixel = screen.PixelAt(0, 0);
        ASSERT_TRUE(pixel.bold);
        ASSERT_TRUE(pixel.italic);
    }

    // Test 3: **bold and *italic* here** — italic nested inside bold
    {
        auto ast = parser->parse("**bold and *italic* here**");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 1u);

        auto& strong = para.children[0];
        ASSERT_EQ(strong.type, NodeType::Strong);
        ASSERT_EQ(strong.children.size(), 3u);
        ASSERT_EQ(strong.children[0].type, NodeType::Text);
        ASSERT_EQ(strong.children[0].text, "bold and ");
        ASSERT_EQ(strong.children[1].type, NodeType::Emphasis);
        ASSERT_EQ(strong.children[1].children[0].text, "italic");
        ASSERT_EQ(strong.children[2].type, NodeType::Text);
        ASSERT_EQ(strong.children[2].text, " here");
    }

    // Test 4: Nested bold+italic renders correct styles at each position
    {
        auto ast = parser->parse("**bold and *italic* here**");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        // "bold and " (positions 0-8) should be bold but not italic
        ASSERT_TRUE(screen.PixelAt(0, 0).bold);
        ASSERT_TRUE(!screen.PixelAt(0, 0).italic);

        // "italic" (positions 9-14) should be bold AND italic
        ASSERT_TRUE(screen.PixelAt(9, 0).bold);
        ASSERT_TRUE(screen.PixelAt(9, 0).italic);

        // " here" (positions 15+) should be bold but not italic
        ASSERT_TRUE(screen.PixelAt(15, 0).bold);
        ASSERT_TRUE(!screen.PixelAt(15, 0).italic);
    }

    // Test 5: *italic with **bold** inside*
    {
        auto ast = parser->parse("*italic with **bold** inside*");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 1u);

        auto& emph = para.children[0];
        ASSERT_EQ(emph.type, NodeType::Emphasis);
        ASSERT_EQ(emph.children.size(), 3u);
        ASSERT_EQ(emph.children[0].type, NodeType::Text);
        ASSERT_EQ(emph.children[0].text, "italic with ");
        ASSERT_EQ(emph.children[1].type, NodeType::Strong);
        ASSERT_EQ(emph.children[1].children[0].text, "bold");
        ASSERT_EQ(emph.children[2].type, NodeType::Text);
        ASSERT_EQ(emph.children[2].text, " inside");
    }

    return 0;
}
