#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Bold CJK characters
    {
        auto ast = parser->parse("**\xe9\x87\x8d\xe8\xa6\x81** task");
        auto& para = ast.children[0];
        ASSERT_EQ(para.type, NodeType::Paragraph);
        ASSERT_EQ(para.children[0].type, NodeType::Strong);

        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "task");
    }

    // Test 2: Italic with accented characters
    {
        auto ast = parser->parse("*\xc3\xa9mphasis*");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children[0].type, NodeType::Emphasis);
        ASSERT_EQ(para.children[0].children[0].text, "\xc3\xa9mphasis");
    }

    // Test 3: List with accented characters
    {
        auto ast = parser->parse("- caf\xc3\xa9\n- na\xc3\xafve");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(2));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "caf");
        ASSERT_CONTAINS(output, "ve");
    }

    // Test 4: Heading with Unicode
    {
        auto ast = parser->parse("# \xc3\x9c" "nic\xc3\xb6" "d\xc3\xa9");
        auto& heading = ast.children[0];
        ASSERT_EQ(heading.type, NodeType::Heading);
        ASSERT_EQ(heading.level, 1);

        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // Just verify no crash, text is rendered
    }

    // Test 5: Link with Unicode text (Japanese katakana)
    {
        auto ast = parser->parse("[\xe3\x83\xaa\xe3\x83\xb3\xe3\x82\xaf](https://example.com)");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children[0].type, NodeType::Link);
        ASSERT_EQ(para.children[0].url, "https://example.com");
    }

    // Test 6: Blockquote with checkmark Unicode
    {
        auto ast = parser->parse("> \xe2\x9c\x85 Done");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Done");
    }

    return 0;
}
