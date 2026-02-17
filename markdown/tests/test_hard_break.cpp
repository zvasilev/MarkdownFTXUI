#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Two trailing spaces produce HardBreak in AST
    {
        auto ast = parser->parse("line1  \nline2");
        // Document > Paragraph > Text("line1"), HardBreak, Text("line2")
        ASSERT_EQ(ast.children.size(), 1u);
        auto& para = ast.children[0];
        ASSERT_EQ(para.type, NodeType::Paragraph);
        ASSERT_TRUE(para.children.size() >= 3u);
        ASSERT_EQ(para.children[0].type, NodeType::Text);
        ASSERT_EQ(para.children[0].text, "line1");
        ASSERT_EQ(para.children[1].type, NodeType::HardBreak);
        ASSERT_EQ(para.children[2].type, NodeType::Text);
        ASSERT_EQ(para.children[2].text, "line2");
    }

    // Test 2: HardBreak renders as separate lines
    {
        auto ast = parser->parse("first  \nsecond");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "first");
        ASSERT_CONTAINS(output, "second");
    }

    // Test 3: HardBreak inside emphasis — no crash
    {
        auto ast = parser->parse("*line1  \nline2*");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "line1");
        ASSERT_CONTAINS(output, "line2");
    }

    // Test 4: Backslash hard break (\ at end of line)
    {
        auto ast = parser->parse("alpha\\\nbeta");
        ASSERT_EQ(ast.children.size(), 1u);
        auto& para = ast.children[0];
        bool has_hb = false;
        for (auto const& child : para.children) {
            if (child.type == NodeType::HardBreak) has_hb = true;
        }
        ASSERT_TRUE(has_hb);
    }

    // Test 5: Multiple hard breaks in sequence
    {
        auto ast = parser->parse("a  \nb  \nc");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(4));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "a");
        ASSERT_CONTAINS(output, "b");
        ASSERT_CONTAINS(output, "c");
    }

    return 0;
}
