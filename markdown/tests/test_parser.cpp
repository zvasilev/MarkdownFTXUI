#include "test_helper.hpp"
#include "markdown/parser.hpp"

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();

    // Test 1: Simple plain text
    {
        auto ast = parser->parse("Hello world");
        ASSERT_EQ(ast.type, NodeType::Document);
        ASSERT_EQ(ast.children.size(), 1u);
        ASSERT_EQ(ast.children[0].type, NodeType::Paragraph);
        ASSERT_EQ(ast.children[0].children.size(), 1u);
        ASSERT_EQ(ast.children[0].children[0].type, NodeType::Text);
        ASSERT_EQ(ast.children[0].children[0].text, "Hello world");
    }

    // Test 2: Two paragraphs
    {
        auto ast = parser->parse("Line one\n\nLine two");
        ASSERT_EQ(ast.type, NodeType::Document);
        ASSERT_EQ(ast.children.size(), 2u);

        ASSERT_EQ(ast.children[0].type, NodeType::Paragraph);
        ASSERT_EQ(ast.children[0].children.size(), 1u);
        ASSERT_EQ(ast.children[0].children[0].type, NodeType::Text);
        ASSERT_EQ(ast.children[0].children[0].text, "Line one");

        ASSERT_EQ(ast.children[1].type, NodeType::Paragraph);
        ASSERT_EQ(ast.children[1].children.size(), 1u);
        ASSERT_EQ(ast.children[1].children[0].type, NodeType::Text);
        ASSERT_EQ(ast.children[1].children[0].text, "Line two");
    }

    // Test 3: Empty input
    {
        auto ast = parser->parse("");
        ASSERT_EQ(ast.type, NodeType::Document);
        ASSERT_EQ(ast.children.size(), 0u);
    }

    // Test 4: Whitespace-only input
    {
        auto ast = parser->parse("   \n\n  ");
        ASSERT_EQ(ast.type, NodeType::Document);
        ASSERT_EQ(ast.children.size(), 0u);
    }

    // Test 5: Soft break (single newline within a paragraph)
    {
        auto ast = parser->parse("Line one\nLine two");
        ASSERT_EQ(ast.type, NodeType::Document);
        ASSERT_EQ(ast.children.size(), 1u);
        ASSERT_EQ(ast.children[0].type, NodeType::Paragraph);
        // Should have: Text("Line one"), SoftBreak, Text("Line two")
        ASSERT_EQ(ast.children[0].children.size(), 3u);
        ASSERT_EQ(ast.children[0].children[0].type, NodeType::Text);
        ASSERT_EQ(ast.children[0].children[0].text, "Line one");
        ASSERT_EQ(ast.children[0].children[1].type, NodeType::SoftBreak);
        ASSERT_EQ(ast.children[0].children[2].type, NodeType::Text);
        ASSERT_EQ(ast.children[0].children[2].text, "Line two");
    }

    // Test 6: 2-arg parse() returns true for valid input
    {
        MarkdownAST ast;
        bool ok = parser->parse("Valid text", ast);
        ASSERT_TRUE(ok);
        ASSERT_EQ(ast.type, NodeType::Document);
        ASSERT_EQ(ast.children.size(), 1u);
    }

    // Test 7: 2-arg parse() populates output AST
    {
        MarkdownAST ast;
        parser->parse("**bold**", ast);
        ASSERT_EQ(ast.children[0].type, NodeType::Paragraph);
        ASSERT_EQ(ast.children[0].children[0].type, NodeType::Strong);
    }

    // Test 8: Hard break (two trailing spaces + newline)
    {
        auto ast = parser->parse("Line one  \nLine two");
        ASSERT_EQ(ast.children.size(), 1u);
        auto& para = ast.children[0];
        ASSERT_EQ(para.type, NodeType::Paragraph);
        bool found_hard_break = false;
        for (auto const& child : para.children) {
            if (child.type == NodeType::HardBreak) found_hard_break = true;
        }
        ASSERT_TRUE(found_hard_break);
    }

    // Test 9: Heading levels 1-6 all parse
    {
        for (int level = 1; level <= 6; ++level) {
            std::string input(static_cast<size_t>(level), '#');
            input += " H";
            MarkdownAST ast;
            bool ok = parser->parse(input, ast);
            ASSERT_TRUE(ok);
            ASSERT_EQ(ast.children[0].type, NodeType::Heading);
            ASSERT_EQ(ast.children[0].level, level);
        }
    }

    return 0;
}
