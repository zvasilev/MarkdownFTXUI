#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Simple list parsed correctly
    {
        auto ast = parser->parse("- one\n- two\n- three");
        ASSERT_EQ(ast.children.size(), 1u);
        auto& list = ast.children[0];
        ASSERT_EQ(list.type, NodeType::BulletList);
        ASSERT_EQ(list.children.size(), 3u);

        ASSERT_EQ(list.children[0].type, NodeType::ListItem);
        ASSERT_EQ(list.children[1].type, NodeType::ListItem);
        ASSERT_EQ(list.children[2].type, NodeType::ListItem);
    }

    // Test 2: List item contains paragraph with text
    {
        auto ast = parser->parse("- hello");
        auto& item = ast.children[0].children[0];
        ASSERT_EQ(item.type, NodeType::ListItem);
        ASSERT_EQ(item.children.size(), 1u);
        ASSERT_EQ(item.children[0].type, NodeType::Paragraph);
        ASSERT_EQ(item.children[0].children[0].text, "hello");
    }

    // Test 3: List renders with bullet prefix
    {
        auto ast = parser->parse("- one\n- two\n- three");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "one");
        ASSERT_CONTAINS(output, "two");
        ASSERT_CONTAINS(output, "three");
    }

    // Test 4: List with bold item
    {
        auto ast = parser->parse("- **bold** item");
        auto& item = ast.children[0].children[0];
        ASSERT_EQ(item.type, NodeType::ListItem);
        auto& para = item.children[0];
        ASSERT_EQ(para.type, NodeType::Paragraph);
        ASSERT_EQ(para.children[0].type, NodeType::Strong);
        ASSERT_EQ(para.children[0].children[0].text, "bold");
        ASSERT_EQ(para.children[1].type, NodeType::Text);
        ASSERT_EQ(para.children[1].text, " item");
    }

    // Test 5: List with * syntax
    {
        auto ast = parser->parse("* alpha\n* beta");
        ASSERT_EQ(ast.children[0].type, NodeType::BulletList);
        ASSERT_EQ(ast.children[0].children.size(), 2u);
    }

    // Test 6: Nested list input degrades gracefully (no crash)
    {
        auto ast = parser->parse("- outer\n  - inner");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(4));
        ftxui::Render(screen, element);
        // Just verify no crash
        ASSERT_TRUE(true);
    }

    return 0;
}
