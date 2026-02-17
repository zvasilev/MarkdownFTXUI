#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: --- parsed as ThematicBreak
    {
        auto ast = parser->parse("above\n\n---\n\nbelow");
        // Document > Paragraph, ThematicBreak, Paragraph
        ASSERT_TRUE(ast.children.size() >= 3u);
        ASSERT_EQ(ast.children[0].type, NodeType::Paragraph);
        ASSERT_EQ(ast.children[1].type, NodeType::ThematicBreak);
        ASSERT_EQ(ast.children[2].type, NodeType::Paragraph);
    }

    // Test 2: *** also produces ThematicBreak
    {
        auto ast = parser->parse("above\n\n***\n\nbelow");
        ASSERT_TRUE(ast.children.size() >= 3u);
        ASSERT_EQ(ast.children[1].type, NodeType::ThematicBreak);
    }

    // Test 3: ___ also produces ThematicBreak
    {
        auto ast = parser->parse("above\n\n___\n\nbelow");
        ASSERT_TRUE(ast.children.size() >= 3u);
        ASSERT_EQ(ast.children[1].type, NodeType::ThematicBreak);
    }

    // Test 4: Thematic break renders — content above and below both visible
    {
        auto ast = parser->parse("above\n\n---\n\nbelow");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(5));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "above");
        ASSERT_CONTAINS(output, "below");
    }

    // Test 5: Multiple thematic breaks — no crash
    {
        auto ast = parser->parse("---\n\n---\n\n---");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(5));
        ftxui::Render(screen, element);
        ASSERT_TRUE(true);
    }

    return 0;
}
