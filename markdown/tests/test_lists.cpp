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

    // Test 6: Nested list renders with indentation
    {
        auto ast = parser->parse("- outer\n  - inner");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(4));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "outer");
        ASSERT_CONTAINS(output, "inner");
    }

    // Test 7: Ordered list parsed correctly
    {
        auto ast = parser->parse("1. first\n2. second\n3. third");
        ASSERT_EQ(ast.children.size(), 1u);
        auto& list = ast.children[0];
        ASSERT_EQ(list.type, NodeType::OrderedList);
        ASSERT_EQ(list.list_start, 1);
        ASSERT_EQ(list.children.size(), 3u);
    }

    // Test 8: Ordered list renders with numbers
    {
        auto ast = parser->parse("1. first\n2. second");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(2));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "1.");
        ASSERT_CONTAINS(output, "first");
        ASSERT_CONTAINS(output, "2.");
        ASSERT_CONTAINS(output, "second");
    }

    // Test 9: Ordered list with custom start
    {
        auto ast = parser->parse("3. alpha\n4. beta");
        ASSERT_EQ(ast.children[0].type, NodeType::OrderedList);
        ASSERT_EQ(ast.children[0].list_start, 3);
    }

    // Test 10: Ordered list with start > 1 renders correct numbers
    {
        auto ast = parser->parse("3. alpha\n4. beta");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(2));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "3.");
        ASSERT_CONTAINS(output, "alpha");
        ASSERT_CONTAINS(output, "4.");
        ASSERT_CONTAINS(output, "beta");
    }

    return 0;
}
