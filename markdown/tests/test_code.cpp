#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Inline code parsed correctly
    {
        auto ast = parser->parse("Use `ls -la`");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 2u);
        ASSERT_EQ(para.children[0].type, NodeType::Text);
        ASSERT_EQ(para.children[0].text, "Use ");
        ASSERT_EQ(para.children[1].type, NodeType::CodeInline);
        ASSERT_EQ(para.children[1].text, "ls -la");
    }

    // Test 2: Standalone inline code
    {
        auto ast = parser->parse("`single`");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 1u);
        ASSERT_EQ(para.children[0].type, NodeType::CodeInline);
        ASSERT_EQ(para.children[0].text, "single");
    }

    // Test 3: Inline code renders with inverted style
    {
        auto ast = parser->parse("normal `code` normal");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "code");

        // "normal " is 7 chars, then inverted code
        ASSERT_TRUE(!screen.PixelAt(0, 0).inverted);
        ASSERT_TRUE(screen.PixelAt(7, 0).inverted);
    }

    // Test 4: Multiple inline code spans
    {
        auto ast = parser->parse("`a` and `b`");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 3u);
        ASSERT_EQ(para.children[0].type, NodeType::CodeInline);
        ASSERT_EQ(para.children[0].text, "a");
        ASSERT_EQ(para.children[1].type, NodeType::Text);
        ASSERT_EQ(para.children[2].type, NodeType::CodeInline);
        ASSERT_EQ(para.children[2].text, "b");
    }

    // Test 5: Inline code in a list item
    {
        auto ast = parser->parse("- use `cmd`");
        auto& item = ast.children[0].children[0];
        auto& para = item.children[0];
        ASSERT_EQ(para.children[1].type, NodeType::CodeInline);
        ASSERT_EQ(para.children[1].text, "cmd");
    }

    // Test 6: Fenced code block with language info
    {
        auto ast = parser->parse("```python\nprint(\"hi\")\n```");
        ASSERT_EQ(ast.children.size(), 1u);
        ASSERT_EQ(ast.children[0].type, NodeType::CodeBlock);
        ASSERT_EQ(ast.children[0].info, "python");
        ASSERT_CONTAINS(ast.children[0].text, "print");
    }

    // Test 7: Fenced code block without language has empty info
    {
        auto ast = parser->parse("```\nsome code\n```");
        ASSERT_EQ(ast.children[0].type, NodeType::CodeBlock);
        ASSERT_TRUE(ast.children[0].info.empty());
    }

    // Test 8: Code block language label renders
    {
        auto ast = parser->parse("```js\nalert(1)\n```");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                            ftxui::Dimension::Fixed(5));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "js");
        ASSERT_CONTAINS(output, "alert");
    }

    return 0;
}
