#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Full mixed document parses correctly
    {
        auto ast = parser->parse(
            "# Plan\n\n"
            "> Focus on **important** tasks\n\n"
            "- Write *code*\n"
            "- Review `tests`\n"
            "- Read [docs](url)\n");

        ASSERT_EQ(ast.type, NodeType::Document);
        // Heading, BlockQuote, BulletList
        ASSERT_EQ(ast.children.size(), 3u);
        ASSERT_EQ(ast.children[0].type, NodeType::Heading);
        ASSERT_EQ(ast.children[0].level, 1);
        ASSERT_EQ(ast.children[1].type, NodeType::BlockQuote);
        ASSERT_EQ(ast.children[2].type, NodeType::BulletList);
    }

    // Test 2: Mixed document renders without crash, all text present
    {
        auto ast = parser->parse(
            "# Plan\n\n"
            "> Focus on **important** tasks\n\n"
            "- Write *code*\n"
            "- Review `tests`\n"
            "- Read [docs](url)\n");

        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(10));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Plan");
        ASSERT_CONTAINS(output, "important");
        ASSERT_CONTAINS(output, "code");
        ASSERT_CONTAINS(output, "tests");
        ASSERT_CONTAINS(output, "docs");
    }

    // Test 3: Heading with bold inside blockquote
    {
        auto ast = parser->parse("# **Bold Heading**");
        auto& heading = ast.children[0];
        ASSERT_EQ(heading.type, NodeType::Heading);
        ASSERT_EQ(heading.children[0].type, NodeType::Strong);
        ASSERT_EQ(heading.children[0].children[0].text, "Bold Heading");
    }

    // Test 4: Bold heading renders bold
    {
        auto ast = parser->parse("# **Bold Heading**");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        // "Bold Heading" starts at position 0, should be bold (from heading + strong)
        ASSERT_TRUE(screen.PixelAt(0, 0).bold);
        ASSERT_CONTAINS(screen.ToString(), "Bold Heading");
    }

    // Test 5: Blockquote containing bold, italic, and code
    {
        auto ast = parser->parse("> **bold** *italic* `code`");
        auto& bq = ast.children[0];
        ASSERT_EQ(bq.type, NodeType::BlockQuote);
        auto& para = bq.children[0];
        ASSERT_EQ(para.type, NodeType::Paragraph);

        // Should have: Strong, Text(" "), Emphasis, Text(" "), CodeInline
        bool has_strong = false, has_emphasis = false, has_code = false;
        for (auto& child : para.children) {
            if (child.type == NodeType::Strong) has_strong = true;
            if (child.type == NodeType::Emphasis) has_emphasis = true;
            if (child.type == NodeType::CodeInline) has_code = true;
        }
        ASSERT_TRUE(has_strong);
        ASSERT_TRUE(has_emphasis);
        ASSERT_TRUE(has_code);
    }

    // Test 6: List with mixed inline content renders
    {
        auto ast = parser->parse(
            "- **bold** item\n"
            "- *italic* item\n"
            "- `code` item\n"
            "- [link](url) item\n");

        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(4));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "bold");
        ASSERT_CONTAINS(output, "italic");
        ASSERT_CONTAINS(output, "code");
        ASSERT_CONTAINS(output, "link");
    }

    // Test 7: Multiple headings at different levels
    {
        auto ast = parser->parse(
            "# H1\n\n## H2\n\n### H3\n\nParagraph\n");

        ASSERT_EQ(ast.children.size(), 4u);
        ASSERT_EQ(ast.children[0].type, NodeType::Heading);
        ASSERT_EQ(ast.children[0].level, 1);
        ASSERT_EQ(ast.children[1].type, NodeType::Heading);
        ASSERT_EQ(ast.children[1].level, 2);
        ASSERT_EQ(ast.children[2].type, NodeType::Heading);
        ASSERT_EQ(ast.children[2].level, 3);
        ASSERT_EQ(ast.children[3].type, NodeType::Paragraph);
    }

    return 0;
}
