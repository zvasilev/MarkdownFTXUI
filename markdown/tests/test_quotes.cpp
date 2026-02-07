#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Simple blockquote parsed correctly
    {
        auto ast = parser->parse("> note");
        ASSERT_EQ(ast.children.size(), 1u);
        auto& bq = ast.children[0];
        ASSERT_EQ(bq.type, NodeType::BlockQuote);
        ASSERT_EQ(bq.children.size(), 1u);
        ASSERT_EQ(bq.children[0].type, NodeType::Paragraph);
        ASSERT_EQ(bq.children[0].children[0].text, "note");
    }

    // Test 2: Blockquote renders with dim styling
    {
        auto ast = parser->parse("> note");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "note");

        // The text after the "│ " prefix should be dim
        // "│ " is 2 chars, then "note" starts at position 2
        ASSERT_TRUE(screen.PixelAt(2, 0).dim);
    }

    // Test 3: Blockquote with bold content
    {
        auto ast = parser->parse("> **important** note");
        auto& bq = ast.children[0];
        ASSERT_EQ(bq.type, NodeType::BlockQuote);
        auto& para = bq.children[0];
        ASSERT_EQ(para.children[0].type, NodeType::Strong);
        ASSERT_EQ(para.children[0].children[0].text, "important");
        ASSERT_EQ(para.children[1].type, NodeType::Text);
        ASSERT_EQ(para.children[1].text, " note");
    }

    // Test 4: Blockquote with bold renders bold + dim
    {
        auto ast = parser->parse("> **important** note");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        // "│ " prefix (2 chars), then "important" starts at pos 2
        ASSERT_TRUE(screen.PixelAt(2, 0).bold);
        ASSERT_TRUE(screen.PixelAt(2, 0).dim);
    }

    // Test 5: Blockquote followed by normal text
    {
        auto ast = parser->parse("> quoted\n\nnormal");
        ASSERT_EQ(ast.children.size(), 2u);
        ASSERT_EQ(ast.children[0].type, NodeType::BlockQuote);
        ASSERT_EQ(ast.children[1].type, NodeType::Paragraph);
    }

    return 0;
}
