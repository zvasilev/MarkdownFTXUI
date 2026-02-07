#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Italic with asterisks
    {
        auto ast = parser->parse("*this matters*");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 1u);
        ASSERT_EQ(para.children[0].type, NodeType::Emphasis);
        ASSERT_EQ(para.children[0].children[0].type, NodeType::Text);
        ASSERT_EQ(para.children[0].children[0].text, "this matters");
    }

    // Test 2: Italic with underscores, mixed with plain text
    {
        auto ast = parser->parse("A _subtle_ point");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 3u);
        ASSERT_EQ(para.children[0].type, NodeType::Text);
        ASSERT_EQ(para.children[0].text, "A ");
        ASSERT_EQ(para.children[1].type, NodeType::Emphasis);
        ASSERT_EQ(para.children[1].children[0].text, "subtle");
        ASSERT_EQ(para.children[2].type, NodeType::Text);
        ASSERT_EQ(para.children[2].text, " point");
    }

    // Test 3: Italic renders with italic style
    {
        auto ast = parser->parse("normal *italic* normal");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "italic");

        // "normal " is 7 chars, then italic text
        ASSERT_TRUE(!screen.PixelAt(0, 0).italic);
        ASSERT_TRUE(screen.PixelAt(7, 0).italic);
    }

    // Test 4: Multiple italic segments
    {
        auto ast = parser->parse("*a* and *b*");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 3u);
        ASSERT_EQ(para.children[0].type, NodeType::Emphasis);
        ASSERT_EQ(para.children[1].type, NodeType::Text);
        ASSERT_EQ(para.children[2].type, NodeType::Emphasis);
    }

    // Test 5: Italic is not bold
    {
        auto ast = parser->parse("*only italic*");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(screen.PixelAt(0, 0).italic);
        ASSERT_TRUE(!screen.PixelAt(0, 0).bold);
    }

    return 0;
}
