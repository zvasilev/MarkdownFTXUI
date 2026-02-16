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

    // Test 11: Empty list item — no crash, 2 items
    {
        auto ast = parser->parse("- \n- text");
        auto& list = ast.children[0];
        ASSERT_EQ(list.type, NodeType::BulletList);
        ASSERT_EQ(list.children.size(), 2u);

        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(2));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "text");
    }

    // Test 12: 3-level nested list — correct structure
    {
        auto ast = parser->parse("- level 1\n  - level 2\n    - level 3");
        auto& l1 = ast.children[0];
        ASSERT_EQ(l1.type, NodeType::BulletList);
        // First item has paragraph + nested list
        auto& item1 = l1.children[0];
        ASSERT_EQ(item1.type, NodeType::ListItem);
        // Find nested list in item1 children
        bool found_nested = false;
        for (auto const& child : item1.children) {
            if (child.type == NodeType::BulletList) {
                found_nested = true;
                // Second level has an item with another nested list
                auto& item2 = child.children[0];
                ASSERT_EQ(item2.type, NodeType::ListItem);
            }
        }
        ASSERT_TRUE(found_nested);

        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(6));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "level 1");
        ASSERT_CONTAINS(output, "level 3");
    }

    // Test 13: Ordered list followed by bullet list — two separate list nodes
    {
        auto ast = parser->parse("1. ordered\n2. items\n\n- bullet\n- items");
        ASSERT_TRUE(ast.children.size() >= 2u);
        ASSERT_EQ(ast.children[0].type, NodeType::OrderedList);
        ASSERT_EQ(ast.children[1].type, NodeType::BulletList);
    }

    // Test 14: List item with link wraps instead of clipping
    {
        auto ast = parser->parse(
            "* [Access Copilot through its SDK:](https://example.com)"
            " Released in technical preview, you can access Copilot"
            " through several languages.");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(60),
                                            ftxui::Dimension::Fixed(4));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        // Full link title must be visible (was truncated before fix)
        ASSERT_CONTAINS(output, "Access Copilot through its SDK:");
        // Description text must wrap, not be clipped
        ASSERT_CONTAINS(output, "Released");
        ASSERT_CONTAINS(output, "languages");
    }

    return 0;
}
