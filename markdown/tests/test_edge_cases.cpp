#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Empty document — no crash
    {
        auto ast = parser->parse("");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // Just verify no crash
    }

    // Test 2: Whitespace only — no crash
    {
        auto ast = parser->parse("   \n\n\n   ");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, element);
    }

    // Test 3: Unclosed bold — best-effort render, no crash
    {
        auto ast = parser->parse("**unclosed bold");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        // cmark-gfm treats unclosed ** as literal text
        ASSERT_CONTAINS(output, "unclosed bold");
    }

    // Test 4: "* * * *" — graceful handling (cmark-gfm may treat as thematic break)
    {
        auto ast = parser->parse("* * * *");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // Just verify no crash
    }

    // Test 5: Ordered list — rendered as best-effort (not supported)
    {
        auto ast = parser->parse("1. ordered");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "ordered");
    }

    // Test 6: Fenced code block — rendered in a border box
    {
        auto ast = parser->parse("```\ncode block\n```");
        ASSERT_EQ(ast.children[0].type, NodeType::CodeBlock);
        ASSERT_CONTAINS(ast.children[0].text, "code block");

        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "code block");
    }

    // Test 7: Nested list — flat rendering, no crash
    {
        auto ast = parser->parse("- nested\n  - list");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "nested");
        ASSERT_CONTAINS(output, "list");
    }

    // Test 8: Very long line — no crash
    {
        std::string long_line(500, 'x');
        auto ast = parser->parse(long_line);
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
    }

    // Test 9: Many paragraphs — no crash
    {
        std::string many_paras;
        for (int i = 0; i < 100; ++i) {
            many_paras += "Paragraph " + std::to_string(i) + "\n\n";
        }
        auto ast = parser->parse(many_paras);
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(200));
        ftxui::Render(screen, element);
    }

    // Test 10: Unclosed link — no crash
    {
        auto ast = parser->parse("[unclosed link(url");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "unclosed");
    }

    // Test 11: Only markers, no content
    {
        auto ast = parser->parse("# \n\n> \n\n- \n");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(5));
        ftxui::Render(screen, element);
    }

    // Test 12: Thematic break (horizontal rule)
    {
        auto ast = parser->parse("above\n\n---\n\nbelow");
        bool found_break = false;
        for (auto const& child : ast.children) {
            if (child.type == NodeType::ThematicBreak) found_break = true;
        }
        ASSERT_TRUE(found_break);

        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(5));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "above");
        ASSERT_CONTAINS(output, "below");
    }

    // Test 13: Image parsed as Image node
    {
        auto ast = parser->parse("![alt text](https://example.com/img.png)");
        auto& para = ast.children[0]; // Paragraph wrapping inline image
        bool found_image = false;
        for (auto const& child : para.children) {
            if (child.type == NodeType::Image) {
                found_image = true;
                ASSERT_CONTAINS(child.url, "example.com");
            }
        }
        ASSERT_TRUE(found_image);

        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "alt text");
    }

    // Test 14: HTML inline rendered as text
    {
        auto ast = parser->parse("before <b>html</b> after");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "before");
        ASSERT_CONTAINS(output, "after");
    }

    // Test 15: Blockquote at max depth — no crash, fallback renders
    {
        builder.set_max_quote_depth(2);
        auto ast = parser->parse("> level 1\n> > level 2\n> > > level 3");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(6));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "level 1");
        // Level 3 exceeds max depth but should still render (as fallback)
        builder.set_max_quote_depth(10); // reset
    }

    // Test 16: Link with empty URL — empty url string
    {
        auto ast = parser->parse("[text]()");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children[0].type, NodeType::Link);
        ASSERT_TRUE(para.children[0].url.empty());
        ASSERT_EQ(para.children[0].children[0].text, "text");
    }

    // Test 17: Very long single word (no spaces) — no crash in flexbox
    {
        std::string long_word(500, 'W');
        auto ast = parser->parse(long_word);
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(10));
        ftxui::Render(screen, element);
        // Verify no crash; content may be clipped
    }

    return 0;
}
