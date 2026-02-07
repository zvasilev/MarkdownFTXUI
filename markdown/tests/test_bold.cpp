#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Bold text parsed correctly
    {
        auto ast = parser->parse("This is **important**");
        ASSERT_EQ(ast.type, NodeType::Document);
        ASSERT_EQ(ast.children.size(), 1u);
        auto& para = ast.children[0];
        ASSERT_EQ(para.type, NodeType::Paragraph);
        ASSERT_EQ(para.children.size(), 2u);
        ASSERT_EQ(para.children[0].type, NodeType::Text);
        ASSERT_EQ(para.children[0].text, "This is ");
        ASSERT_EQ(para.children[1].type, NodeType::Strong);
        ASSERT_EQ(para.children[1].children.size(), 1u);
        ASSERT_EQ(para.children[1].children[0].type, NodeType::Text);
        ASSERT_EQ(para.children[1].children[0].text, "important");
    }

    // Test 2: Fully bold paragraph
    {
        auto ast = parser->parse("**full bold**");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 1u);
        ASSERT_EQ(para.children[0].type, NodeType::Strong);
        ASSERT_EQ(para.children[0].children[0].text, "full bold");
    }

    // Test 3: Bold with underscore syntax
    {
        auto ast = parser->parse("__also bold__");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 1u);
        ASSERT_EQ(para.children[0].type, NodeType::Strong);
        ASSERT_EQ(para.children[0].children[0].text, "also bold");
    }

    // Test 4: Bold renders with bold style
    {
        auto ast = parser->parse("normal **bold** normal");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "normal");
        ASSERT_CONTAINS(output, "bold");

        // "normal" at position 0 should not be bold
        ASSERT_TRUE(!screen.PixelAt(0, 0).bold);

        // Find where "bold" starts (after "normal ")
        // "normal " is 7 chars, then bold text
        ASSERT_TRUE(screen.PixelAt(7, 0).bold);
    }

    // Test 5: Multiple bold segments
    {
        auto ast = parser->parse("**a** and **b**");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 3u);
        ASSERT_EQ(para.children[0].type, NodeType::Strong);
        ASSERT_EQ(para.children[1].type, NodeType::Text);
        ASSERT_EQ(para.children[1].text, " and ");
        ASSERT_EQ(para.children[2].type, NodeType::Strong);
    }

    return 0;
}
